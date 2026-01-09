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

#include <float.h> 
#include <math.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zcard_plugin.h"
#include "vca_dac_calibration.h"

// Calibration for the DAC channels that control the SSI2190.  This is for
// five DAC channels.  A sixth DAC channel is CtrlRef, the zero reference
// for each channel.
// This is basically worst-case limiting to match DAC channels.
// General idea for the calibration:
// 1. user measures Vmin and Vmax for each of six 12-bit DAC channels
// 2. Software will find the maximum low voltage of each DAC channel
// 3. Software will find the minimum high voltage of each DAC channel
// 4. Software calculates the DAC minimum for each channel to match the maximum low
// 5. Software calculates the DAC maximum for each channel to match the minimum high
// 6. Software calculates a linear interpolation table for each channel going from low to high


// Calibration constants
#define CORRECTION_TABLE_SIZE (DAC2190_MAX_CODE + 1)

// Nominal voltage range for when calibration fails
#define NOMINAL_MAX_VOLTAGE 2.5f
#define VSLOPE_NOMINAL (NOMINAL_MAX_VOLTAGE / DAC2190_MAX_CODE)

#define VSLOPE_MIN_EXPECTED (VSLOPE_NOMINAL * 0.9f)
#define VSLOPE_MAX_EXPECTED (VSLOPE_NOMINAL * 1.1f)



// Convert a V-target to a DAC code based on the physical channel limits
// (Kept separate to avoid re-implementing logic)
static uint16_t voltage_to_code(float V_target, float V_slope, float V_min_intercept) {
    float raw_code = (V_target - V_min_intercept) / V_slope;

    // clamp to legal value
    if (raw_code < 0.0f) raw_code = 0.0f;
    if (raw_code > DAC2190_MAX_CODE) raw_code = DAC2190_MAX_CODE;
    
    return (uint16_t)roundf(raw_code);
}



struct dac_device* init_dac_calibration(const struct dac_channel_descriptor *channels, size_t num_channels) {
  struct dac_device *dac_device = (struct dac_device*) malloc(sizeof(struct dac_device));
  if (dac_device == NULL) {
    ERROR("memmory alloc error for DAC calibration");
    return NULL;
  }

  memcpy(dac_device->channels_descriptor, channels, sizeof(struct dac_channel_descriptor) * num_channels);
  return dac_device;
}


/* init_vca_dac2190_calibration
 *
 * translate to init_dac_calibration- maybe this be later refactored to parameterize.
 * Not necessary for now.
 */
struct dac_device* init_vca_dac2190_calibration(const struct dac_channel_descriptor channels[DAC2190_NUM_CHANNELS]) {
  return init_dac_calibration(channels, DAC2190_NUM_CHANNELS);
}



int calculate_calibration_parameters(struct dac_device *dac_device) {
  int use_fallback_range = 0;

  // 1. Pre-calculate physical slope (volts/code) and intercept for each channel
  for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
    dac_device->channels_coeffs[i].Vslope = 
      (dac_device->channels_descriptor[i].Vmax_measured - dac_device->channels_descriptor[i].Vmin_measured) / DAC2190_MAX_CODE;
    if (dac_device->channels_coeffs[i].Vslope < VSLOPE_MIN_EXPECTED ||
        dac_device->channels_coeffs[i].Vslope > VSLOPE_MAX_EXPECTED) {
      WARN("channel %d unexpected Vslope %.3f.  Falling back to non-calibrated values", i, dac_device->channels_coeffs[i].Vslope * 1000);
      use_fallback_range = 1;
    }
  }

  // --- STEP 2 & 3: Find device's Vlow_device and high_device
  dac_device->Vlow_device = 0.0f;
  dac_device->Vhigh_device = FLT_MAX;
  for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
    if (dac_device->channels_descriptor[i].Vmin_measured > dac_device->Vlow_device) {
      dac_device->Vlow_device = dac_device->channels_descriptor[i].Vmin_measured;
    }
    if (dac_device->channels_descriptor[i].Vmax_measured < dac_device->Vhigh_device) {
      dac_device->Vhigh_device = dac_device->channels_descriptor[i].Vmax_measured;
    }
  }


  if (dac_device->Vhigh_device <= dac_device->Vlow_device) {
    // Fallback Scenario: The calibrated range is invalid
    WARN("a channel's low voltage (%.3fV) is greater than a channel's high voltage (%.3fV)", 
         dac_device->Vhigh_device, dac_device->Vlow_device);
    use_fallback_range = 1;
  }

  if (use_fallback_range == 1) {
    INFO("falling back to raw DAC range [0, %u] for all channels.", DAC2190_MAX_CODE);

    // Set Dmin/Dmax to the raw full range for the LUT generation (Steps 4 & 5 Fallback)
    for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
      dac_device->channels_coeffs[i].Dmin_calc = 0;
      dac_device->channels_coeffs[i].Dmax_calc = DAC2190_MAX_CODE;
    }
        
    return use_fallback_range; // Calibration is done, using fallback limits
  }
    
  // Valid Calibration Path
  INFO("Calibration success. Global Operating Range: [%.3fV, %.3fV]", 
         dac_device->Vlow_device, dac_device->Vhigh_device);

  // --- STEPS 4 & 5: Calculate New Dmin and Dmax (Valid Path) ---
  for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
    // Step 4: Find the DAC code (Dmin) that produces dac_device->Vlow_device
    dac_device->channels_coeffs[i].Dmin_calc =
      voltage_to_code(dac_device->Vlow_device,
                      dac_device->channels_coeffs[i].Vslope,
                      dac_device->channels_descriptor[i].Vmin_measured);

    // Step 5: Find the DAC code (Dmax) that produces dac_device->Vhigh_device
    dac_device->channels_coeffs[i].Dmax_calc =
      voltage_to_code(dac_device->Vhigh_device,
                      dac_device->channels_coeffs[i].Vslope,
                      dac_device->channels_descriptor[i].Vmin_measured);
        
    INFO("INFO: Ch %d: D_min_calc=%u, D_max_calc=%u",
         i,
         dac_device->channels_coeffs[i].Dmin_calc, 
         dac_device->channels_coeffs[i].Dmax_calc);
  }

  return use_fallback_range;
}


/* Generates 4096-entry lookup table for each DAC channel on the device.
 *
 * This function performs Step 6 of the calibration procedure.
 * Return zero on success.
 */
int generate_channel_calibrated_codes(struct dac_device *dac_device) {
    
  for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
    uint16_t Dmin_calc = dac_device->channels_coeffs[i].Dmin_calc;
    uint16_t Dmax_calc = dac_device->channels_coeffs[i].Dmax_calc;

    // 1. Allocate memory for the calibrated table
    uint16_t *lut = (uint16_t*) malloc(CORRECTION_TABLE_SIZE * sizeof(uint16_t));
    if (lut == NULL) {
      ERROR("Failed to allocate memory for LUT for Channel %d", i);
      // cleanup any allocated resources
      for (int j = 0; j < i; ++j) {
        free(dac_device->calibrated_codes[j]);
        dac_device->calibrated_codes[j] = NULL;
      }
      dac_device->calibrated_codes[i] = NULL;
      return -1; // Indicate memory failure
    }

    // Success
    dac_device->calibrated_codes[i] = lut;

    // Define the span and constant factors for the calculation
    const float D_span = (float)Dmax_calc - (float)Dmin_calc;
    const float D_norm_max_f = (float)DAC2190_MAX_CODE;

    // 2. Populate the table entries
    // First, ensure endpoint correctness:
    lut[0] = __builtin_bswap16(Dmin_calc | (dac_device->channels_descriptor[i].wire_prefix << 8));
    // then the rest of the entries from 1..DAC2190_MAX_CODE:
    for (uint16_t k = 1; k <= DAC2190_MAX_CODE; ++k) {
      // k is the Normalized Code (0 to 4095) that the user writes
      // Calculate the fractional position in the normalized range (0.0 to 1.0)
      float fractional_position = (float)k / D_norm_max_f;

      // Linear mapping: D_actual = D_min + (Fractional Position * D_span)
      float actual_code_f = (float)Dmin_calc + (fractional_position * D_span);

      // Clamp and round the result to the nearest integer DAC code
      // Since we clamped Dmin/Dmax, this should naturally stay in range, 
      // but we rely on roundf and cast to uint16_t.
      uint16_t actual_code = (uint16_t)roundf(actual_code_f);

      // Final safety clamp for robustness (even though rounding should handle it)
      if (actual_code > DAC2190_MAX_CODE) {
        actual_code = DAC2190_MAX_CODE;
      }
            
      // take the actual code and prefix first four bits with wire prefix.
      // Now it's ready to take to the DAC.
      uint16_t tmp =  (dac_device->channels_descriptor[i].wire_prefix << 8) | (actual_code & 0x0FFF);
      lut[k] = __builtin_bswap16(tmp);

      if (k < 10) {
        INFO("wire ready %d: %4hx --> %4hx", i, actual_code, lut[k]);
      }

    }

    INFO("Channel %d LUT generated. Maps normalized code [0, %u] to actual code [%u, %u]", 
         i, 
         DAC2190_MAX_CODE, 
         lut[0],  
         lut[DAC2190_MAX_CODE] 
         );
  }
    
  INFO("All %d DAC Lookup Tables successfully calculated and stored",
       DAC2190_NUM_CHANNELS);

  for (int i = 0; i < DAC2190_NUM_CHANNELS; ++i) {
    INFO("Values: %d", i);
    for (int k = 0; k < 5; ++k) {
      INFO("wire ready %d: %4x --> %4hx", i, k, dac_device->calibrated_codes[i][k]);
    }

    INFO("...");

    for (int k = DAC2190_MAX_CODE - 4; k <= DAC2190_MAX_CODE; ++k) {
      INFO("wire ready %d: %4x --> %4hx", i, k, dac_device->calibrated_codes[i][k]);
    }
  }

  return 0; // Success
}



// Utility function to clean up allocated memory
void free_vca_dac_calibration(struct dac_device *dac_device) {
  for (int i = 0; i < DAC2190_NUM_CHANNELS; i++) {
    if (dac_device->calibrated_codes[i] != NULL) {
      free(dac_device->calibrated_codes[i]);
      dac_device->calibrated_codes[i] = NULL;
    }
  }
}
