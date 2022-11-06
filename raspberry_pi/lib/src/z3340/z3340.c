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

#define SPI_MODE 1
#define PCA9555_I2C_ADDRESS 0x20

struct z3340_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port0;
  uint8_t pca9555_port1;
};

static const uint8_t port0_addr = 0x02;
static const uint8_t port1_addr = 0x03;
static const uint8_t config_port0_addr = 0x06;
static const uint8_t config_port1_addr = 0x07;
static const uint8_t config_port_as_output = 0x00;


void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_I2C_ADDRESS;
  assert(slot >= 0 && slot < 8);
  struct z3340_card *z3340 = (struct z3340_card*)calloc(1, sizeof(struct z3340_card));
  if (z3340 == NULL) {
    return NULL;
  }

  z3340->zhost = zhost;
  z3340->slot = slot;
  z3340->pca9555_port0 = 0x00;
  z3340->pca9555_port1 = 0x00;

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
  error += i2cWriteByteData(z3340->i2c_handle, port0_addr, z3340->pca9555_port0);
  error += i2cWriteByteData(z3340->i2c_handle, port1_addr, z3340->pca9555_port1);

  if (error) {
    ERROR("z3340: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(z3340->i2c_handle);
    free(z3340);
    return NULL;
  }

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



// six audio channels mapped to an 8 channel DAC (yup, two unused DAC channels)
static const int channel_map[] = { 0, 1, 3, 4, 5, 7 };
static int16_t previous_samples[6] = { 0 };


int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;

  spi_channel = set_spi_interface(zcard->zhost, SPI_MODE, zcard->slot);

  for (int i = 0; i < 6; ++i) {
    if (previous_samples[i] != samples[i] ) {
      previous_samples[i] = samples[i];

      samples_to_dac[0] = (channel_map[i] << 4) |
        ((uint16_t) samples[i]) >> 11;

      samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;

      spiWrite(spi_channel, samples_to_dac, 2);
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


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  return 0;
}

