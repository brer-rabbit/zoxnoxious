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

#include "z5524.h"
#include "tune_utils.h"

//
// Tuning section
// Put all the tuning stuff down here.  A lot of it.
// This tunes the VCOs on both chips and the VCF to 1 volt/octave.
//


static const uint8_t tune_gpio_port0_data = 0x00;
static const uint8_t tune_gpio_port1_data = 0b11000000; // wave select: saw/tri off
// tune_dac_state is in spiWrite order
// set these values when tuning the 2130
static const char tune_2130_dac_state_as3394[][2] = { { 0x00, 0x00 }, // vco mix: 2130
                                                      { 0x10, 0x00 }, // pwm: off
                                                      { 0x2f, 0xff }, // final gain
                                                      { 0x30, 0x00 }, // vcf mod
                                                      { 0x40, 0x00 }, // resonance
                                                      { 0x50, 0x00 }, // 2130 mod vca
                                                      { 0x60, 0x00 }, // 3394 vco exp freq
                                                      { 0x7f, 0xff } }; // vcf: open

static const char tune_2130_dac_state_ssi2130[][2] = { { 0x00, 0x00 }, // tri mix
                                                       { 0x17, 0xff }, // linear: mid-range
                                                       { 0x20, 0x00 }, // exp pitch
                                                       { 0x37, 0xff }, // pwm 50%
                                                       { 0x40, 0x00 }, // sine vca
                                                       { 0x50, 0x00 }, // 3394 mod vca
                                                       { 0x6f, 0xff }, // pulse mix: 100%
                                                       { 0x70, 0x00 } }; // saw mix

// set these values when tuning the 3394 vco
static const char tune_3394_vco_dac_state_as3394[][2] = { { 0x0f, 0xff }, // vco mix: 3394
                                                          { 0x17, 0xff }, // pwm: 50%
                                                          { 0x2f, 0xff }, // final gain
                                                          { 0x30, 0x00 }, // vcf mod
                                                          { 0x40, 0x00 }, // resonance
                                                          { 0x50, 0x00 }, // 2130 mod vca
                                                          { 0x60, 0x00 }, // 3394 vco exp freq
                                                          { 0x7f, 0xff } }; // vcf: open

static const char tune_3394_dac_state_ssi2130[][2] = { { 0x00, 0x00 }, // tri mix
                                                       { 0x17, 0xff }, // linear: mid-range
                                                       { 0x20, 0x00 }, // exp pitch
                                                       { 0x37, 0xff }, // pwm
                                                       { 0x40, 0x00 }, // sine vca
                                                       { 0x50, 0x00 }, // 3394 mod vca
                                                       { 0x60, 0x00 }, // pulse mix: off
                                                       { 0x70, 0x00 } }; // saw mix

// set these values when tuning the 3394 vcf (ssi2130 as per above)
static const char tune_3394_vcf_dac_state_as3394[][2] = { { 0x0f, 0xff }, // vco mix: 3394
                                                          { 0x10, 0x00 }, // pwm: 0%
                                                          { 0x2f, 0xff }, // final gain
                                                          { 0x30, 0x00 }, // vcf mod
                                                          { 0x4f, 0xff }, // res: crank it!
                                                          { 0x50, 0x00 }, // 2130 mod vca
                                                          { 0x67, 0xff }, // 3394 vco exp freq
                                                          { 0x7f, 0xff } }; // vcf: open


// tune frequency dac values: the DAC values to run through for a tuning request.
// Revese the byte order before calling spiWrite and logical or with DAC line.
// These values are used for both the 2130 and 3394.
static const uint16_t tune_freq_dac_values[] = { 0x0000,
                                                0x0200,
                                                0x0400,
                                                0x0600,
                                                0x0800,
                                                0x0a00,
                                                0x0c00,
                                                0x0e00,
                                                0x0fff };

// Tuning values
static const double tuning_vco_initial_frequency_target = 27.5;  // A0
static const double tuning_vcf_initial_frequency_target = 13.75; // A-1
static const double expected_dac_ssi2130_values_per_octave = 512; // for 12 bits / 8 octave range
static const double expected_dac_as3394_vco_values_per_octave = 682.667; // for 12 bits / 6 octave range
static const double expected_dac_as3394_vcf_values_per_octave = 409.6; // for 12 bits / 10 octave range

static void write_dac_lines(struct z5524_card *zcard, const char dac[][2], int num_lines, int chip_select);
static int create_correction_table(struct tunable_5524 *tunable, double initial_frequency, char dac_line);



/** tunereq_save_state
 * The DAC and gpio can be restored based on existing state, so no need to save those.
 * Setup tuning structures
 */
int tunereq_save_state(void *zcard_plugin) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;
  int error;


  // structure prep
  memset(&zcard->tunables[TUNE_SSI2130_VCO].tuning_points, 0, sizeof(struct tune_point) * NUM_TUNING_POINTS);
  memset(&zcard->tunables[TUNE_AS3394_VCO].tuning_points, 0, sizeof(struct tune_point) * NUM_TUNING_POINTS);
  memset(&zcard->tunables[TUNE_AS3394_VCF].tuning_points, 0, sizeof(struct tune_point) * NUM_TUNING_POINTS);
  zcard->tune_target = TUNE_SSI2130_VCO; // start with the 2130
  zcard->tuning_index = 0;

  // set DAC values for approp for SSI2130 tuning
  write_dac_lines(zcard, tune_2130_dac_state_ssi2130, DAC_CHANNELS, spi_channel_ssi2130);
  write_dac_lines(zcard, tune_2130_dac_state_as3394, DAC_CHANNELS, spi_channel_as3394);

  // set any state necessary on the gpio -- all modulations off, outputs set
  error = i2cWriteByteData(zcard->i2c_handle, port0_addr, tune_gpio_port0_data);
  error += i2cWriteByteData(zcard->i2c_handle, port1_addr, tune_gpio_port1_data);
  if (error) {
    ERROR("z5524: tunereq_save_state: error writing to I2C bus handle %d\n", zcard->i2c_handle);
    return TUNE_COMPLETE_FAILED;
  }

  return TUNE_CONTINUE;
}



tune_status_t tunereq_set_point(void *zcard_plugin) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;
  const uint8_t led_bit = 0x80;
  int spi_channel;
  char dac_values[2];

  // This defines the test point data to send to the DAC.
  // Determine which DAC line we're writing to and OR that into the right place.
  dac_values[0] = tune_freq_dac_values[ zcard->tuning_index ] >> 8;
  dac_values[1] = tune_freq_dac_values[ zcard->tuning_index ];

  switch(zcard->tune_target) {
  case TUNE_SSI2130_VCO:
    spi_channel = set_spi_interface(zcard->zhost, spi_channel_ssi2130, SPI_MODE, zcard->slot);
    dac_values[0] |=  ssi2130_vco_dac;
    break;
  case TUNE_AS3394_VCO:
    if (zcard->tuning_index == 0) {
      // first tune point on 3394 vco
      write_dac_lines(zcard, tune_3394_dac_state_ssi2130, DAC_CHANNELS, spi_channel_ssi2130);
      write_dac_lines(zcard, tune_3394_vco_dac_state_as3394, DAC_CHANNELS, spi_channel_as3394);
    }
    spi_channel = set_spi_interface(zcard->zhost, spi_channel_as3394, SPI_MODE, zcard->slot);
    dac_values[0] |=  as3394_vco_dac;
    break;
  case TUNE_AS3394_VCF:
    if (zcard->tuning_index == 0) {
      write_dac_lines(zcard, tune_3394_dac_state_ssi2130, DAC_CHANNELS, spi_channel_ssi2130);
      write_dac_lines(zcard, tune_3394_vcf_dac_state_as3394, DAC_CHANNELS, spi_channel_as3394);
    }
    spi_channel = set_spi_interface(zcard->zhost, spi_channel_as3394, SPI_MODE, zcard->slot);
    dac_values[0] |=  as3394_vcf_dac;
    break;
  default:
    return TUNE_COMPLETE_FAILED;
  }

  spiWrite(spi_channel, dac_values, 2);

  // obligatory "I'm tuning so blink the light"
  i2cWriteByteData(zcard->i2c_handle, port0_addr,
                   zcard->tuning_index & 0x1 ? tune_gpio_port0_data | led_bit : tune_gpio_port0_data);

  return TUNE_CONTINUE;
}



tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;
  struct tune_point *tp;

  if (tuning_measurement->samples == 0) {
    ERROR("tuning measure zero samples for tuning point %d / target %d",
          zcard->tuning_index, zcard->tune_target);
    return TUNE_COMPLETE_FAILED;
  }

  // calculate expected frequency values

  switch (zcard->tune_target) {
  case TUNE_SSI2130_VCO:
    tp = &zcard->tunables[TUNE_SSI2130_VCO].tuning_points[ zcard->tuning_index ];
    tp->actual_dac = tune_freq_dac_values[ zcard->tuning_index ];
    tp->frequency = tuning_measurement->frequency;
    tp->expected_dac = octave_delta(tuning_measurement->frequency, tuning_vco_initial_frequency_target) *
      expected_dac_ssi2130_values_per_octave;
    break;
  case TUNE_AS3394_VCO:
    tp = &zcard->tunables[TUNE_AS3394_VCO].tuning_points[ zcard->tuning_index ];
    tp->actual_dac = tune_freq_dac_values[ zcard->tuning_index ];
    tp->frequency = tuning_measurement->frequency;
    tp->expected_dac = octave_delta(tuning_measurement->frequency, tuning_vco_initial_frequency_target) *
      expected_dac_as3394_vco_values_per_octave;
    break;
  case TUNE_AS3394_VCF:
    tp = &zcard->tunables[TUNE_AS3394_VCF].tuning_points[ zcard->tuning_index ];
    tp->actual_dac = tune_freq_dac_values[ zcard->tuning_index ];
    tp->frequency = tuning_measurement->frequency;
    tp->expected_dac = octave_delta(tuning_measurement->frequency, tuning_vcf_initial_frequency_target) * expected_dac_as3394_vcf_values_per_octave;
    break;
  case TUNE_TARGET_LENGTH:
    ERROR("unexpected tune measure report");
    return TUNE_COMPLETE_FAILED;
  }

  // increment to next point
  if (++zcard->tuning_index == NUM_TUNING_POINTS) {
    zcard->tuning_index = 0;
    INFO("card %d tune target %d complete", zcard->slot, zcard->tune_target);
    if (++zcard->tune_target == TUNE_TARGET_LENGTH) {
      return TUNE_COMPLETE_SUCCESS;
    }
  }

  return TUNE_CONTINUE;
}



int tunereq_restore_state(void *zcard_plugin) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;


  // restore gpio
  int error = i2cWriteByteData(zcard->i2c_handle, port0_addr, zcard->pca9555_port[0]);
  error += i2cWriteByteData(zcard->i2c_handle, port1_addr, zcard->pca9555_port[1]);
  if (error) {
      ERROR("i2c write error on restore state from tuning");
      return TUNE_COMPLETE_FAILED;
  }

  // restore by setting to invalid state
  for (int i = 0; i < CHIP_SELECTS; ++i) {
    for (int j = 0; j < DAC_CHANNELS; ++j) {
      zcard->previous_samples[i][j] = -1;
    }
  }


  if (zcard->tune_target == TUNE_TARGET_LENGTH) {
    create_correction_table(&zcard->tunables[TUNE_SSI2130_VCO], tuning_vco_initial_frequency_target, ssi2130_vco_dac);
    create_correction_table(&zcard->tunables[TUNE_AS3394_VCO], tuning_vco_initial_frequency_target, as3394_vco_dac);
    create_correction_table(&zcard->tunables[TUNE_AS3394_VCF], tuning_vcf_initial_frequency_target, as3394_vcf_dac);
  }

  return TUNE_COMPLETE_SUCCESS;
}


static void write_dac_lines(struct z5524_card *zcard, const char dac[][2], int num_lines, int chip_select) {
  int spi_channel = set_spi_interface(zcard->zhost, chip_select, SPI_MODE, zcard->slot);

  for (int i = 0; i < num_lines; ++i) {
    spiWrite(spi_channel, (char*)dac[i], 2);
  }
}


// TODO: this really needs to be refactored and put to tune_utils

static int create_correction_table(struct tunable_5524 *tunable, double initial_frequency, char dac_line) {

  double oct_delta = octave_delta(tunable->tuning_points[1].frequency,
                                  tunable->tuning_points[0].frequency);

  double dac_delta_per_octave = (TWELVE_BITS / (NUM_TUNING_POINTS - 1)) / oct_delta;
  double low_freq_dac_value = dac_value_for_freq(initial_frequency,
                                                 dac_delta_per_octave,
                                                 tunable->tuning_points[0].frequency);
  double slope;

  // overwrite the expected_dac and actual_dac values of the initial entry--
  // basically do a linear interp and move the y-intercept
  tunable->tuning_points[0].expected_dac = 0.0; //user expects dac 0 for tuning_initial_frequency_target
  tunable->tuning_points[0].actual_dac = 0.5 + low_freq_dac_value;

  int tuned_index;
  for (int i = 0; i < NUM_TUNING_POINTS - 1; ++i) {
    for (tuned_index = tunable->tuning_points[i].expected_dac + 0.5;
         tuned_index < tunable->tuning_points[i+1].expected_dac - 0.5 &&
           tuned_index < TWELVE_BITS;
         ++tuned_index) {
      // y = mx + b; we have b as zcard->tuning_points[i].actual_dac
      // compute m and x
      slope = (tunable->tuning_points[i+1].actual_dac - tunable->tuning_points[i].actual_dac) /
        (tunable->tuning_points[i+1].expected_dac - tunable->tuning_points[i].expected_dac);
      int x = tuned_index - (int)(tunable->tuning_points[i].expected_dac + 0.5);
      int y = slope * x + tunable->tuning_points[i].actual_dac;
      tunable->calibration_table[tuned_index] = y > 0 ? y : 0;
    }
  }

  // fill in remaining elements of table from tuned_index-1 onward.
  INFO("dac line %c corrected to index: %d", ((unsigned char)dac_line >> 4) + '0', tuned_index);
  for (int i = tuned_index - 1; i < TWELVE_BITS; ++i) {
    tunable->calibration_table[i] = tunable->calibration_table[tuned_index - 1];
  }

  // cleanup table
  for (int i = 0; i < TWELVE_BITS; ++i) {
    if (tunable->calibration_table[i] > TWELVE_BITS - 1) {
      tunable->calibration_table[i] = 0xff0f | dac_line;
    }
    else {
      unsigned char upper, lower;
      upper = tunable->calibration_table[i];
      lower = dac_line | (0xf & tunable->calibration_table[i] >> 8);
      tunable->calibration_table[i] = ((int16_t)upper) << 8 | (int16_t)lower;
    }
  }

  return 0;
}
