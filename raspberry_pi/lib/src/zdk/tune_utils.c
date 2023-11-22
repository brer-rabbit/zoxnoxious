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

#include <math.h>

#include "zcard_plugin.h"
#include "tune_utils.h"


static const double inv_log2 = 1 / log(2);


double octave_delta(double f1, double f2) {
  return log(f1 / f2) * inv_log2;
}


int16_t dac_value_for_freq(double target_freq, double dac_delta_per_octave, double ref_freq) {
  double freq_ratio = target_freq / ref_freq;
  double dac_value = log(freq_ratio) * inv_log2 * dac_delta_per_octave;
  return dac_value + 0.5;
}




int create_correction_table(struct tunable *tunable, double initial_frequency) {
  if (tunable == NULL || tunable->tune_points_size < 2) {
    ERROR("not creating correction table due to insufficient tune points or tunable");
    return 1;
  }

  double oct_delta = octave_delta(tunable->tune_points[1].frequency, tunable->tune_points[0].frequency);

  double dac_delta_per_octave = (tunable->dac_size / (tunable->tune_points_size - 1)) / oct_delta;
  double low_freq_dac_value = dac_value_for_freq(initial_frequency,
                                                 dac_delta_per_octave,
                                                 tunable->tune_points[0].frequency);
  double slope;

  // overwrite the expected_dac and actual_dac values of the initial entry--
  // basically do a linear interp and move the y-intercept
  tunable->tune_points[0].expected_dac = 0.0; // user expects dac 0 for initial frequency target
  tunable->tune_points[0].actual_dac = 0.5 + low_freq_dac_value;

  int tuned_index;
  for (int i = 0; i < tunable->tune_points_size - 1; ++i) {
    for (tuned_index = tunable->tune_points[i].expected_dac + 0.5;
         tuned_index < tunable->tune_points[i+1].expected_dac - 0.5 && tuned_index < tunable->dac_size;
         ++tuned_index) {
      // y = mx + b; we have b as zcard->tune_points[i].actual_dac
      // compute m and x
      slope = (tunable->tune_points[i+1].actual_dac - tunable->tune_points[i].actual_dac) /
        (tunable->tune_points[i+1].expected_dac - tunable->tune_points[i].expected_dac);
      int x = tuned_index - (int)(tunable->tune_points[i].expected_dac + 0.5);
      int y = slope * x + tunable->tune_points[i].actual_dac;
      tunable->dac_calibration_table[tuned_index] = y > 0 ? y : 0; // non-negative only
    }
  }

  // fill in remaining elements of table from tuned_index-1 onward.
  // this is essentially resolution thrown away.
  INFO("correcting remaining from index: %d", tuned_index);
  for (int i = tuned_index - 1; i < tunable->dac_size; ++i) {
    tunable->dac_calibration_table[i] = tunable->dac_calibration_table[tuned_index - 1];
  }

  return 0;

}


int prep_correction_table_ad5328(struct tunable *tunable, uint8_t dac_line) {

  // cleanup table
  for (int i = 0; i < tunable->dac_size; ++i) {
    if (tunable->dac_calibration_table[i] > tunable->dac_size - 1) {
      tunable->dac_calibration_table[i] = 0xff0f | dac_line;
    }
    else {
      uint8_t upper, lower;
      upper = tunable->dac_calibration_table[i];
      lower = dac_line | (0xf & tunable->dac_calibration_table[i] >> 8);
      tunable->dac_calibration_table[i] = ((int16_t)upper) << 8 | (int16_t)lower;
    }
  }

  return 0;
}
