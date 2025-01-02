/* Copyright 2025 Kyle Farrell
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

#include "poledancer.h"
#include "tune_utils.h"

//
// Tuning section. All the tuning stuff here.
// Current state is VCF frequency tuning.
// Future idea may be to tune rez level.  Other params?
//


// for tuning: all modulation off, muxes disabled.  QVCA Invert should be set.
static const uint8_t tune_gpio_port0_data = 0x11;
static const uint8_t tune_gpio_port1_data = 0x02;
static const uint8_t port1_led_bit = 0x01;

// tune_dac_state is in spiWrite order
// set these values when tuning the 2140; spi channel 0
static const char tune_dac_state_2140[][2] = { { 0x00, 0x00 }, // vcf cutoff
                                               { 0x40, 0x00 }, // source1 audio
                                               { 0x50, 0x00 }, // source1 mod
                                               { 0x60, 0x00 }, // source2 audio
                                               { 0x70, 0x00 } }; // source2 mod

// set these values when on the 2190 (well, qctrl is on the 2140, crosses over) using spi channel 1
static const char tune_dac_state_2190[][2] = { { 0x1f, 0xff }, // pole4
                                               { 0x20, 0x00 }, // pole2
                                               { 0x30, 0x00 }, // pole3
                                               { 0x40, 0x00 }, // pole1
                                               { 0x6f, 0xff }, // Qctrl CV
                                               { 0x70, 0x00 } }; // Dry Ctrl


// tune frequency dac values: the DAC values to run through for a tuning request.
// Bytes are reversed from spiWrite and need the DAC line to call spiWrite with.
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
static const double tuning_vcf_initial_frequency_target = 13.75; // A-1
static const double expected_dac_vcf_values_per_octave = 409.6; // for 12 bits / 10 octave range: 13.75Hz - 14080Hz



/** tunereq_save_state
 * The DAC and gpio can be restored based on existing state, so no need to save those.
 * Setup tuning structures
 */
int tunereq_save_state(void *zcard_plugin) {
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;
  int error;
  int spi_channel;

  zcard->tuning_index = 0;

  // set DAC values for tuning
  spi_channel = set_spi_interface(zcard->zhost, spi_channel_cs0, SPI_MODE, zcard->slot);
  for (int i = 0; i < DAC_CHANNELS_CS0; ++i) {
    spiWrite(spi_channel, (char*)tune_dac_state_2140[i], 2);
  }
  spi_channel = set_spi_interface(zcard->zhost, spi_channel_cs1, SPI_MODE, zcard->slot);
  for (int i = 0; i < DAC_CHANNELS_CS1; ++i) {
    spiWrite(spi_channel, (char*)tune_dac_state_2190[i], 2);
  }

  INFO("poledancer tune init");

  // set any state necessary on the gpio -- all modulations off, outputs set
  error = i2cWriteByteData(zcard->i2c_handle, port0_addr, tune_gpio_port0_data);
  error += i2cWriteByteData(zcard->i2c_handle, port1_addr, tune_gpio_port1_data);
  if (error) {
    ERROR("tunereq_save_state: error writing to I2C bus handle %d\n", zcard->i2c_handle);
    return TUNE_COMPLETE_FAILED;
  }

  return TUNE_CONTINUE;
}




tune_status_t tunereq_set_point(void *zcard_plugin) {
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;
  int spi_channel;
  char dac_values[2];

  // This defines the test point data to send to the DAC.
  // Determine which DAC line we're writing to and OR that into the right place.
  dac_values[0] = cutoff_cv_channel << 4 |
    tune_freq_dac_values[ zcard->tuning_index ] >> 8;
  dac_values[1] = tune_freq_dac_values[ zcard->tuning_index ];

  spi_channel = set_spi_interface(zcard->zhost, spi_channel_cs0, SPI_MODE, zcard->slot);
  spiWrite(spi_channel, dac_values, 2);

  // obligatory "I'm tuning so blink the light"
  i2cWriteByteData(zcard->i2c_handle, port1_addr,
                   zcard->tuning_index & 0x1 ?
                   tune_gpio_port1_data | port1_led_bit : tune_gpio_port1_data);

  INFO("poledancer tune set point complete");

  return TUNE_CONTINUE;
}




tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;
  struct tune_point *tp;

  if (tuning_measurement->samples == 0) {
    ERROR("tuning measure zero samples for tuning point %d",
          zcard->tuning_index);
    //return TUNE_COMPLETE_FAILED;
  }

  // calculate expected frequency values
  tp = &zcard->tunable.tune_points[ zcard->tuning_index ];
  tp->actual_dac = tune_freq_dac_values[ zcard->tuning_index ];
  tp->frequency = tuning_measurement->frequency;
  tp->expected_dac = octave_delta(tuning_measurement->frequency,
                                  tuning_vcf_initial_frequency_target) *
    expected_dac_vcf_values_per_octave;


  INFO("poledancer tune measure point complete");

  // increment to next point
  if (++zcard->tuning_index == NUM_TUNING_POINTS) {
    return TUNE_COMPLETE_SUCCESS;
  }

  return TUNE_CONTINUE;
}



int tunereq_restore_state(void *zcard_plugin) {
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;


  // restore gpio
  int error = i2cWriteByteData(zcard->i2c_handle, port0_addr, zcard->pca9555_port[0]);
  error += i2cWriteByteData(zcard->i2c_handle, port1_addr, zcard->pca9555_port[1]);
  if (error) {
      ERROR("i2c write error on restore state from tuning");
      return TUNE_COMPLETE_FAILED;
  }

  // restore by setting to invalid state
  for (int i = 0; i < DAC_CHANNELS_CS0; ++i) {
    zcard->previous_samples_cs0[i] = -1;
  }
  for (int i = 0; i < DAC_CHANNELS_CS1; ++i) {
    zcard->previous_samples_cs1[i] = -1;
  }


  // then calculate corrections
  if (zcard->tuning_index >= NUM_TUNING_POINTS) {
    create_correction_table(&zcard->tunable, tuning_vcf_initial_frequency_target);
    prep_correction_table_ad5328(&zcard->tunable, cutoff_cv_channel << 4);
  }
  else {
    return TUNE_COMPLETE_FAILED;
  }

  return TUNE_COMPLETE_SUCCESS;
}
