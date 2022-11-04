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

#include <stdio.h>

#include "zcard_plugin.h"


void* init_zcard(int slot) {
  return NULL;
}


void free_zcard(void *zcard_plugin) {
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


#define SPI_RATE 12000000

// six audio channels mapped to an 8 channel DAC (yup, two unused DAC channels)
static const int channel_map[] = { 0, 1, 3, 4, 5, 7 };
static int16_t previous_samples[6] = { 0 };


int process_samples(void *zcard_plugin, const int16_t *samples) {
  char samples_to_dac[2];
  int spi_channel;

  if ((spi_channel = spiOpen(0, SPI_RATE, 1)) < 0) {
    return 1;
  }

  for (int i = 0; i < 6; ++i) {
    if (previous_samples[i] != samples[i] ) {
      previous_samples[i] = samples[i];

      samples_to_dac[0] = (channel_map[i] << 4) |
        ((uint16_t) samples[i]) >> 11;

      samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;

      spiWrite(spi_channel, samples_to_dac, 2);
    }

  }

  spiClose(spi_channel);


  /*
    printf("z3340: samples: %p %5hd %5hd %5hd %5hd %5hd %5hd\n",
           (void*) samples,
           samples[0], samples[1], samples[2],
           samples[3], samples[4], samples[5]);
  }
  */

  return 0;
}


int process_midi(void *zcard_plugin, uint8_t *midi_message, size_t size) {
  return 0;
}


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  return 0;
}

