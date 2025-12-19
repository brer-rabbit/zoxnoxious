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

#ifndef VCA_DAC_CALIBRATION_H
#define VCA_DAC_CALIBRATION_H

#include <stdint.h>

// Calibration constants
#define DAC2190_NUM_CHANNELS 6
#define DAC2190_MAX_CODE 4095


// Data structure holding the measured analog voltage limits for a single DAC channel.
// These values must correspond to the physical output voltages produced at
// DAC code 0 (Vmin_measured) and DAC full-scale (Vmax_measured).
//
// The user of the library supplies an array of these structures to the
// initialization function. The data is copied internally and is not modified
// after initialization.

struct dac_channel_descriptor {
  // analog voltage characteristics
  float Vmin_measured;
  float Vmax_measured;
  uint16_t wire_prefix;
};


// Internal structure holding derived calibration coefficients for a DAC channel.
// These values are computed from dac_channel_descriptor during the calibration process
// and define the digital sub-range used to implement worst-case channel limiting.
//
// This structure is owned and maintained by the calibration library and should
// not be modified by user code.

struct dac_channel_coeffs {
  float Vslope; // (Vmax - Vmin) / DAC2190_MAX_CODE
  // digital value calculations
  uint16_t Dmin_calc; // New DAC code that produces dac_device->Vlow_device or 0 (fallback)
  uint16_t Dmax_calc; // New DAC code that produces dac_device->Vhigh_device or 4095 (fallback)
} ;


// Opaque-like device structure representing a calibrated DAC instance.
// This structure owns all memory associated with the calibration, including
// per-channel lookup tables.
//
// Most fields are maintained internally by the calibration library. User code
// typically interacts with this structure by indexing calibrated_codes[channel]
// with a normalized DAC code (0 .. DAC2190_MAX_CODE) to obtain a hardware-ready,
// corrected DAC code.
//
// Fields are populated as follows:
//  - channels_descriptor: initialized during init_vca_dac2190_calibration()
//  - channels_coeffs and Vlow_device / Vhigh_device: populated by
//    calculate_calibration_parameters()
//  - calibrated_codes: allocated and populated by
//    generate_channel_calibrated_codes()
//
// All memory owned by this structure must be released by calling free_vca_dac_calibration()

struct dac_device {
  // global operating range for DAC to implement worst-case limiting
  float Vlow_device;
  float Vhigh_device;
  struct dac_channel_descriptor channels_descriptor[DAC2190_NUM_CHANNELS];
  struct dac_channel_coeffs channels_coeffs[DAC2190_NUM_CHANNELS];

  // Array of pointers to the Lookup Tables (4096 entries each)
  uint16_t *calibrated_codes[DAC2190_NUM_CHANNELS];
};


/* init_vca_dac2190_calibration
 *
 * pass in an array of dac_channel_descriptor for initialization.  The array is copied and
 * used for calibration.
 */
struct dac_device* init_vca_dac2190_calibration(const struct dac_channel_descriptor channels[DAC2190_NUM_CHANNELS]);
void calculate_calibration_parameters(struct dac_device *dac_device);
int generate_channel_calibrated_codes(struct dac_device *dac_device);
void free_vca_dac_calibration(struct dac_device *dac_device);

#endif // VCA_DAC_CALIBRATION_H
