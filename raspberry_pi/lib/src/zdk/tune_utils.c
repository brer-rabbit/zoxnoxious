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


double octave_delta(double f1, double f2) {
  return log(f1 / f2) * inv_log2;
}


int16_t dac_value_for_freq(double target_freq, double dac_delta_per_octave, double ref_freq) {
  double freq_ratio = target_freq / ref_freq;
  double dac_value = log(freq_ratio) * inv_log2 * dac_delta_per_octave;
  return dac_value + 0.5;
}
