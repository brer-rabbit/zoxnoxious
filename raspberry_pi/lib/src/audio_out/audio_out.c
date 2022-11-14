/* Copyright 2022 Kyle Farrell
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "zcard_plugin.h"


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20

// DAC: MCP4822
#define SPI_MODE 0


struct audio_out_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];
  int16_t previous_samples[2];
};

static const uint8_t port0_addr = 0x02;
static const uint8_t port1_addr = 0x03;
static const uint8_t config_port0_addr = 0x06;
static const uint8_t config_port1_addr = 0x07;
static const uint8_t config_port_as_output = 0x00;
static const uint8_t dac_channel[2] = { 0x90, 0x10 };


void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  assert(slot >= 0 && slot < 8);
  struct audio_out_card *audio_out = (struct audio_out_card*)calloc(1, sizeof(struct audio_out_card));
  if (audio_out == NULL) {
    return NULL;
  }

  audio_out->zhost = zhost;
  audio_out->slot = slot;
  audio_out->pca9555_port[0] = 0x00;
  audio_out->pca9555_port[1] = 0x00;

  audio_out->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (audio_out->i2c_handle < 0) {
    ERROR("audio_out: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }
  

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(audio_out->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(audio_out->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(audio_out->i2c_handle, port0_addr, audio_out->pca9555_port[0]);
  error += i2cWriteByteData(audio_out->i2c_handle, port1_addr, audio_out->pca9555_port[1]);

  if (error) {
    ERROR("audio_out: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(audio_out->i2c_handle);
    free(audio_out);
    return NULL;
  }

  return audio_out;
}


void free_zcard(void *zcard_plugin) {
}


char* get_plugin_name() {
  return "Audio Out";
}

struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = 2;
  props->spi_mode = 1; // think this is actually 0
  return props;
}


int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct audio_out_card *zcard = (struct audio_out_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;

  spi_channel = set_spi_interface(zcard->zhost, SPI_MODE, zcard->slot);

  for (int i = 0; i < 2; ++i) {
    if (zcard->previous_samples[i] != samples[i] ) {
      zcard->previous_samples[i] = samples[i];

      samples_to_dac[0] = dac_channel[i] | ((uint16_t) samples[i]) >> 11;
      samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;

      spiWrite(spi_channel, samples_to_dac, 2);
      /*
      WARN("audio out: channel %d 0x%4X : 0x%2X 0x%2X",
           i,
           samples[i],
           samples_to_dac[0], samples_to_dac[1]);
      */
    }
  }

  return 0;
}


int process_midi(void *zcard_plugin, uint8_t *midi_message, size_t size) {
  return 0;
}


struct midi_program_to_gpio {
  int port;
  uint8_t gpio_reg; // gpio register zero or one?
  uint8_t set_bits;  // mask to OR
  uint8_t clear_bits; // mask to AND
};

// array indexed by MIDI program number
static const struct midi_program_to_gpio midi_program_to_gpio[] = {
// reg, reg addr,   or-mask,    and-mask
  { 0, port0_addr, 0b00000000, 0b11011111 }, // prog 0 - card A mix1 off
  { 0, port0_addr, 0b00100000, 0b11111111 }, // prog 1 - card A mix1 on
  { 0, port0_addr, 0b00000000, 0b11110111 }, // prog 2 - card B mix1 off 
  { 0, port0_addr, 0b00001000, 0b11111111 }, // prog 3 - card B mix1 on  
  { 0, port0_addr, 0b00000000, 0b11111011 }, // prog 4 - card C mix1 off
  { 0, port0_addr, 0b00000100, 0b11111111 }, // prog 5 - card C mix1 on 
  { 0, port0_addr, 0b00000000, 0b11101111 }, // prog 6 - card D mix1 off
  { 0, port0_addr, 0b00010000, 0b11111111 }, // prog 7 - card D mix1 on  
  { 0, port0_addr, 0b00000000, 0b11111101 }, // prog 8 - card E mix1 off
  { 0, port0_addr, 0b00000010, 0b11111111 }, // prog 9 - card E mix1 on  
  { 0, port0_addr, 0b00000000, 0b11111110 }, // prog 10 - card F mix1 off
  { 0, port0_addr, 0b00000001, 0b11111111 }, // prog 11 - card F mix1 on  
  { 0, port0_addr, 0b01000000, 0b11111111 }, // prog 12 - MIX1 VCA Right
  { 0, port0_addr, 0b00000000, 0b10111111 }, // prog 13 - MIX2 VCA Right
  { 0, port0_addr, 0b00000000, 0b01111111 }, // prog 14 - MIX2 VCA Left
  { 0, port0_addr, 0b10000000, 0b11111111 }, // prog 15 - MIX1 VCA Left
  // V0.2:
  { 1, port1_addr, 0b01000010, 0b01111110 }, // prog 18 - card E mix 2
  { 1, port1_addr, 0b11000010, 0b11111110 }, // prog 19 - card F mix 2
  { 1, port1_addr, 0b01000000, 0b01111100 }, // prog 20 - card A mix 2
  { 1, port1_addr, 0b11000000, 0b11111100 }, // prog 21 - card B mix 2
  { 1, port1_addr, 0b01000001, 0b01111101 }, // prog 16 - card C mix 2
  { 1, port1_addr, 0b11000001, 0b11111101 }, // prog 17 - card D mix 2
  { 1, port1_addr, 0b00000000, 0b00111100 }, // prog 22 - Mix2 off
  /*
  V0.3 data:
  { 1, port1_addr, 0b10010000, 0b10011111 }, // prog 16 - card A mix 2
  { 1, port1_addr, 0b10110000, 0b10111111 }, // prog 17 - card B mix 2
  { 1, port1_addr, 0b00010000, 0b00011111 }, // prog 18 - card C mix 2
  { 1, port1_addr, 0b00110000, 0b00111111 }, // prog 19 - card D mix 2
  { 1, port1_addr, 0b01010000, 0b01011111 }, // prog 20 - card E mix 2
  { 1, port1_addr, 0b01110000, 0b01111111 }, // prog 21 - card F mix 2
  { 1, port1_addr, 0b00000000, 0b11101111 }  // prog 22 - Mix2 off
  */
};


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct audio_out_card *zcard = (struct audio_out_card*)zcard_plugin;

  INFO("audio out: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

    INFO("audio out: prog 0x%X: wrote 0x%X to port %d",
         program_number, zcard->pca9555_port[ prog_gpio_entry->port ],
         prog_gpio_entry->gpio_reg);
  }
  else {
    WARN("audio out: unexpected midi program number: 0x%X", program_number);
  }

  return error;
}

