/* Copyright 2024 Kyle Farrell
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

#ifndef POLEDANCER_H
#define POLEDANCER_H

#include "zcard_plugin.h"
#include "tune_utils.h"


#define NUM_TUNING_POINTS 9
#define TWELVE_BITS 4096
#define CHIP_SELECTS 2
#define DAC_CHANNELS_CS0 5
#define DAC_CHANNELS_CS1 6

// 2x DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1


struct poledancer_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples_cs0[DAC_CHANNELS_CS0];
  int16_t previous_samples_cs1[DAC_CHANNELS_CS1];

  // tuning params
  struct tunable tunable;
  int tuning_index; // which dac value to apply on the next set and record the measurement
};


extern const int spi_channel_cs0;
extern const int spi_channel_cs1;

extern const uint8_t port0_addr;
extern const uint8_t port1_addr;
extern const uint8_t config_port0_addr;
extern const uint8_t config_port1_addr;
extern const uint8_t config_port_as_output;
extern const uint8_t cutoff_cv_channel;

void create_linear_tuning(int dac_channel, int num_elements, int16_t *table);


#endif // POLEDANCER_H
