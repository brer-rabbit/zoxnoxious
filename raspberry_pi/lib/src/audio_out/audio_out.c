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
  uint8_t pca9555_port0;
  uint8_t pca9555_port1;
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
  audio_out->pca9555_port0 = 0x00;
  audio_out->pca9555_port1 = 0x00;

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
  error += i2cWriteByteData(audio_out->i2c_handle, port0_addr, audio_out->pca9555_port0);
  error += i2cWriteByteData(audio_out->i2c_handle, port1_addr, audio_out->pca9555_port1);

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


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  return 0;
}

