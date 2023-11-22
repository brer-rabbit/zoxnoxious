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

#ifndef TUNE_UTILS_H
#define TUNE_UTILS_H

#include <stdint.h>


struct tune_point {
  int actual_dac;
  double frequency;
  double expected_dac;
};

struct tunable {
  struct tune_point *tune_points;
  int tune_points_size;
  int16_t *dac_calibration_table;
  int dac_size;
};


/** octave_delta
 * compute the octave difference between two frequencies
 */
double octave_delta(double f1, double f2);

/** dac_value_for_hz
 *
 * Return the dac value closest to Hz frequency
 * target_freq : frequency of interest
 * dac_delta_per_octave : return from measured_dac_delta_per_octave
 * ref_freq : low frequency for dac_delta_per_octave
 */

int16_t dac_value_for_freq(double target_freq, double dac_delta_per_octave, double ref_freq);


/** create_correction_table
 *
 * take a tunable pointer with a full set of tuning_points and fill in
 * the dac_calibration_table.
 */
int create_correction_table(struct tunable *tunable, double initial_frequency);


/* prep_correction_table_ad5328
 *
 * prep the correction table for a 12-bit AD5328 DAC.  Take the dac_line as an input.  Rewrite
 * values to 12-bit values, clipping anything greater than 12 bits.  Write the result as it
 * would go straight to the dac_line via spiWrite.
 */
int prep_correction_table_ad5328(struct tunable *tunable, uint8_t dac_line);

#endif // TUNE_UTILS_H
