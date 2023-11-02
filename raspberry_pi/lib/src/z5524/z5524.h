/* Copyright 2023 Kyle Farrell
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

#ifndef Z5524_H
#define Z5524_H

#include "zcard_plugin.h"
#include "tune_utils.h"


#define NUM_TUNING_POINTS 9
#define TWELVE_BITS 4096
#define CHIP_SELECTS 2
#define DAC_CHANNELS 8

// 2x DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1


// this enum ordering is relevant in process samples
typedef enum {
  TUNE_SSI2130_VCO,
  TUNE_AS3394_VCO,
  TUNE_AS3394_VCF,
  TUNE_TARGET_LENGTH
} tune_target_t;

struct z5524_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples[CHIP_SELECTS][DAC_CHANNELS];

  // tuning params
  struct tunable tunables[TUNE_TARGET_LENGTH];
  tune_target_t tune_target; // index to tunables
  int tuning_index; // which dac value to apply on the next set and record the measurement
};


extern const int spi_channel_ssi2130;
extern const int spi_channel_as3394;

extern const uint8_t port0_addr;
extern const uint8_t port1_addr;
extern const uint8_t config_port0_addr;
extern const uint8_t config_port1_addr;
extern const uint8_t config_port_as_output;
extern const uint8_t startup_hard_sync_value;
extern const uint16_t startup_as3394_high_freq;
extern const uint16_t startup_as3394_vco_dac_line;

extern const char ssi2130_vco_dac;
extern const char as3394_vco_dac;
extern const char as3394_vcf_dac;

void create_linear_tuning(int dac_channel, int num_elements, int16_t *table);


#endif // Z5524_H
