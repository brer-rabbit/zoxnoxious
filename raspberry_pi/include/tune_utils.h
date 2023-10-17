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

/** measured_dac_delta_per_octave
 *
 * calculate the DAC slope for one octave.  Inputs:
 * f_low : low frequency for range
 * f_high : high frequency for range
 * dac_low : dac value for low frequency
 * dac_high : dac value for high frequency
 *
 * Returns the correct slope as measured/expected
 */

double measured_dac_delta_per_octave(double f_low, double f_high, int dac_low, int dac_high);


/** corrected_slope
 *
 * actual_dac_values_per_octave : dac value to increase one octave measured
 * expected_dac_values_per_octave : dac value to increase one octave expected
 */

double corrected_slope(double actual_dac_values_per_octave, double expected_dac_values_per_octave);


/** calc_dac_corrections
 *
 * with the given slope and the previous correction, write num elements to corrections.
 * slope : the corrected slope
 * prev_correction : value from previous segment, or zero to init
 * num_elements : number of elements to write
 * corrections : table of corrections to write num_elements to IN RANGE OF 1..num_elements (not 0..num_elements-1!)
 */
int calc_dac_corrections(double slope, int prev_correction, int num_elements, int16_t *corrections);


/** dac_value_for_hz
 *
 * Return the dac value closest to Hz frequency
 * target_freq : frequency of interest
 * dac_delta_per_octave : return from measured_dac_delta_per_octave
 * ref_freq : low frequency for dac_delta_per_octave
 * dac_offset : dac value for ref_freq
 */

int16_t dac_value_for_hz(double target_freq, double dac_delta_per_octave, double ref_freq, int16_t dac_offset);

#endif // TUNE_UTILS_H
