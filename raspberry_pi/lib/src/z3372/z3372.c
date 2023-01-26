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

#include <assert.h>
#include <stdio.h>

#include "zcard_plugin.h"


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20

// DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1

struct z3372_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples[7];
};

static const uint8_t port0_addr = 0x02;
static const uint8_t port1_addr = 0x03;
static const uint8_t config_port0_addr = 0x06;
static const uint8_t config_port1_addr = 0x07;
static const uint8_t config_port_as_output = 0x00;

// six audio channels mapped to an 8 channel DAC (yup, two unused DAC channels)
static const int channel_map[] = { 0x00, 0x10, 0x30, 0x40, 0x50, 0x70 };



void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  int spi_channel;
  // AD5328: control register
  char dac_ctrl0_reg[2] = { 0b11110000, 0b00000000 };  // full reset, data and control
  char dac_ctrl1_reg[2] = { 0b10000000, 0b00000000 };  // power on, gain 1x, unbuffered Vref input

  assert(slot >= 0 && slot < 8);
  struct z3340_card *z3340 = (struct z3340_card*)calloc(1, sizeof(struct z3340_card));
  if (z3340 == NULL) {
    return NULL;
  }

  z3340->zhost = zhost;
  z3340->slot = slot;
  z3340->pca9555_port[0] = 0x00;
  z3340->pca9555_port[1] = 0x00;

  z3340->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (z3340->i2c_handle < 0) {
    ERROR("z3340: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(z3340->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(z3340->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(z3340->i2c_handle, port0_addr, z3340->pca9555_port[0]);
  error += i2cWriteByteData(z3340->i2c_handle, port1_addr, z3340->pca9555_port[1]);

  if (error) {
    ERROR("z3340: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(z3340->i2c_handle);
    free(z3340);
    return NULL;
  }

  // configure DAC
  spi_channel = set_spi_interface(zhost, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);


  return z3340;
}


void free_zcard(void *zcard_plugin) {
  struct z3340_card *z3340 = (struct z3340_card*)zcard_plugin;

  if (zcard_plugin) {
    if (z3340->i2c_handle >= 0) {
      // TODO: turn off LED
      i2cClose(z3340->i2c_handle);
    }
    free(z3340);
  }
}

char* get_plugin_name() {
  return "Zoxnoxious 3340";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = 6;
  props->spi_mode = 0;
  return props;
}



int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;

  spi_channel = set_spi_interface(zcard->zhost, SPI_MODE, zcard->slot);

  for (int i = 0; i < 6; ++i) {
    if (zcard->previous_samples[i] != samples[i] ) {
      zcard->previous_samples[i] = samples[i];

      // DAC write:
      // bits 15-0:
      // 0 A2 A1 A0 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
      // MSB zero specifies DAC data.  Next three bits are DAC address.  Final 12 are data.
      // Given a 16-bit signed input, write it to a 12-bit signed values.
      // Any negative value clips to zero.
      if (samples[i] >= 0) {
        samples_to_dac[0] = channel_map[i] | ((uint16_t) samples[i]) >> 11;
        samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;
      }
      else {
        samples_to_dac[0] = channel_map[i] | (uint16_t) 0;
        samples_to_dac[1] = (uint16_t) 0;
      }

      spiWrite(spi_channel, samples_to_dac, 2);

#ifdef DAC_DEBUG
      // test DAC channel is at 0x60; mirror dac channel 0x40 for test
      if (i == 3) {
        samples_to_dac[0] |= 0x20;
        spiWrite(spi_channel, samples_to_dac, 2);
      }
#endif
        
    }
  }

  /*
    INFO("z3340: samples: %p %5hd %5hd %5hd %5hd %5hd %5hd\n",
           (void*) samples,
           samples[0], samples[1], samples[2],
           samples[3], samples[4], samples[5]);
  */

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
  { 0, port0_addr, 0b00000000, 0b11101111 }, // prog 0 - sync neg off
  { 0, port0_addr, 0b00010000, 0b11111111 }, // prog 1 - sync neg on
  { 1, port1_addr, 0b00000000, 0b11111101 }, // prog 2 - mix1 pulse off
  { 1, port1_addr, 0b00000010, 0b11111111 }, // prog 3 - mix1 pulse on
  { 1, port1_addr, 0b00000000, 0b11111110 }, // prog 4 - mix1 comp off
  { 1, port1_addr, 0b00000001, 0b11111111 }, // prog 5 - mix1 comp on
  { 1, port1_addr, 0b00000000, 0b10111111 }, // prog 6 - mix2 pulse off
  { 1, port1_addr, 0b01000000, 0b11111111 }, // prog 7 - mix2 pulse on
  { 1, port1_addr, 0b00000000, 0b01111111 }, // prog 8 - ext mod pwm off
  { 1, port1_addr, 0b10000000, 0b11111111 }, // prog 9 - ext mod pwm on
  { 0, port0_addr, 0b00000000, 0b11011111 }, // prog 10 - ext mod to fm off
  { 0, port0_addr, 0b00100000, 0b11111111 }, // prog 11 - ext mod to fm on
  { 0, port0_addr, 0b00000000, 0b01111111 }, // prog 12 - linear fm off
  { 0, port0_addr, 0b10000000, 0b11111111 }, // prog 13 - linear fm on
  { 1, port1_addr, 0b00000000, 0b11101111 }, // prog 14 - mix2 saw off
  { 1, port1_addr, 0b00010000, 0b11111111 }, // prog 15 - mix2 saw on
  { 0, port0_addr, 0b00000000, 0b10111111 }, // prog 16 - sync pos off
  { 0, port0_addr, 0b01000000, 0b11111111 }, // prog 17 - sync pos on
  { 1, port1_addr, 0b00000000, 0b11110011 }, // prog 18 - mix1 saw off
  { 1, port1_addr, 0b00001000, 0b11111011 }, // prog 19 - mix1 saw low
  { 1, port1_addr, 0b00000100, 0b11110111 }, // prog 20 - mix1 saw med
  { 1, port1_addr, 0b00001100, 0b11111111 }, // prog 21 - mix1 saw high
  { 0, port0_addr, 0b00000000, 0b11111000 }, // prog 22 - ext select card1 out1
  { 0, port0_addr, 0b00000100, 0b11111100 }, // prog 23 - ext select card1 out2
  { 0, port0_addr, 0b00000010, 0b11111010 }, // prog 24 - ext select card2 out1
  { 0, port0_addr, 0b00000110, 0b11111110 }, // prog 25 - ext select card3 out1
  { 0, port0_addr, 0b00000001, 0b11111001 }, // prog 26 - ext select card4 out1
  { 0, port0_addr, 0b00000101, 0b11111101 }, // prog 27 - ext select card5 out1
  { 0, port0_addr, 0b00000011, 0b11111011 }, // prog 28 - ext select card6 out1
  { 0, port0_addr, 0b00000111, 0b11111111 }  // prog 29 - ext select card7 out1
};

int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;

  INFO("Z3340: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

  }
  else {
    WARN("Z3340: unexpected midi program number: 0x%X", program_number);
  }

  return error;
}

