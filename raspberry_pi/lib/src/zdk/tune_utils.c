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

#include "tune_utils.h"


static const double inv_log2 = 1 / log(2);


double measured_dac_delta_per_octave(double f_low, double f_high, int dac_low, int dac_high) {
  double octave_delta = log(f_high / f_low) * inv_log2;
  return (dac_high - dac_low + 1) / octave_delta;
}

double corrected_slope(double actual_dac_values_per_octave, double expected_dac_values_per_octave) {
  return actual_dac_values_per_octave / expected_dac_values_per_octave;
}



int calc_dac_corrections(double slope, int prev_correction, int num_elements, int16_t *corrections) {
  double corrected_val;

  for (int i = 1; i <= num_elements; ++i) {
    corrected_val = i * slope + prev_correction;
    corrections[i] = corrected_val + 0.5;
  }
  return 0;
}


int16_t dac_value_for_hz(double target_freq, double dac_delta_per_octave, double ref_freq, int16_t ref_freq_dac_value) {
  double freq_ratio = target_freq / ref_freq;
  double dac_value = log(freq_ratio) * inv_log2 * dac_delta_per_octave;
  return (dac_value + 0.5) + ref_freq_dac_value;
}
