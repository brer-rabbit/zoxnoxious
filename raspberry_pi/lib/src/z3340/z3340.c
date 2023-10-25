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

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "zcard_plugin.h"
#include "tune_utils.h"


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20

// DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1
#define SPI_CHANNEL 0 // chip select zero
#define NUM_DAC_CHANNELS 6
#define NUM_TUNING_POINTS 9
#define TWELVE_BITS 4096

struct z3340_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples[NUM_DAC_CHANNELS];

  // tuning params
  int tuning_point; // maintain state between tuning calls
  struct tune_point tuning_points[NUM_TUNING_POINTS];
  int tuning_complete;
  int16_t freq_tuned[TWELVE_BITS];
};


static const uint8_t port0_addr = 0x02;
static const uint8_t port1_addr = 0x03;
static const uint8_t config_port0_addr = 0x06;
static const uint8_t config_port1_addr = 0x07;
static const uint8_t config_port_as_output = 0x00;

// six audio channels mapped to an 8 channel DAC (yup, two unused DAC channels)
static const int channel_map[] = { 0x00, 0x10, 0x30, 0x40, 0x50, 0x70 };


//
// tuning params
//
static const uint8_t tune_gpio_port0_data = 0x00;
static const uint8_t tune_gpio_port1_data = 0x02; // output1 pulse -- not actually necessary
// tune_dac_state is in spiWrite order
static const char tune_dac_state[][2] = { { 0x00, 0x00 }, // freq
                                          { 0x10, 0x00 }, // sync level
                                          { 0x37, 0xff }, // pulse width: 50%
                                          { 0x40, 0x00 }, // tri vca
                                          { 0x50, 0x00 }, // ext mod
                                          { 0x77, 0xff } }; // linear: mid-range

// tune_freq_dac_values: the DAC values to run through for a tuning request:
// all on frequency CV, each step is up by 1.25 volt (512 decimal).
// Reverse byte order before calling spiWrite.
static const int16_t tune_freq_dac_values[] = { 0x0000, // DAC 0V
                                                0x0200, // DAC 1.25V
                                                0x0400, // DAC 2.5V
                                                0x0600, // DAC 3.75V
                                                0x0800, // DAC 5V
                                                0x0a00, // DAC 6.25V
                                                0x0c00, // DAC 7.5V
                                                0x0e00, // DAC 8.75V
                                                0x0fff }; // DAC 10V

static const double tuning_initial_frequency_target = 27.5;
static const double expected_dac_values_per_octave = 409.6; // for 12 bits / 10 octave range

static void create_linear_tuning(int dac_channel, int num_elements, int16_t *table);


//
// slew rate limit the triangle vca
//
static const int slew_limit = 3000;
static const int triangle_vca_id = 3;



void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  int spi_channel;
  // AD5328: control register
  char dac_ctrl0_reg[2] = { 0b11110000, 0b00000000 };  // full reset, data and control
  char dac_ctrl1_reg[2] = { 0b10000000, 0b00000000 };  // power on, gain 1x, unbuffered Vref input

  assert(slot >= 0 && slot < 8);
  struct z3340_card *z3340 = (struct z3340_card*)calloc(1, sizeof(struct z3340_card));
  if (z3340 == NULL) {
    return NULL;
  }

  z3340->zhost = zhost;
  z3340->slot = slot;
  z3340->pca9555_port[0] = 0x00;
  z3340->pca9555_port[1] = 0x00;

  z3340->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (z3340->i2c_handle < 0) {
    ERROR("z3340: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(z3340->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(z3340->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(z3340->i2c_handle, port0_addr, z3340->pca9555_port[0]);
  error += i2cWriteByteData(z3340->i2c_handle, port1_addr, z3340->pca9555_port[1]);

  if (error) {
    ERROR("z3340: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(z3340->i2c_handle);
    free(z3340);
    return NULL;
  }

  // configure DAC
  spi_channel = set_spi_interface(zhost, SPI_CHANNEL, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);


  // set DAC prev values to unallowed value
  for (int i = 0; i < NUM_DAC_CHANNELS; ++i) {
    z3340->previous_samples[i] = -1;
  }

  // tuning -- start with untuned / linear
  create_linear_tuning(channel_map[0], TWELVE_BITS, z3340->freq_tuned);

  return z3340;
}



void free_zcard(void *zcard_plugin) {
  struct z3340_card *z3340 = (struct z3340_card*)zcard_plugin;

  if (zcard_plugin) {
    if (z3340->i2c_handle >= 0) {
      // TODO: turn off LED
      i2cClose(z3340->i2c_handle);
    }
    free(z3340);
  }
}

char* get_plugin_name() {
  return "Zoxnoxious 3340";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = NUM_DAC_CHANNELS;
  props->spi_mode = 0;
  return props;
}



int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;
  int dac_channel = 0;

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);

  // this is broken up, rather crudely, to filter the freq CV and
  // triangle VCA amount as special cases.  If not for that a single
  // loop would do.
  // Freq CV, use correction table:

  if (zcard->previous_samples[dac_channel] != samples[dac_channel]) {
    if (samples[dac_channel] >= 0) {
      // samples[dac_channel] is 16 bits.  Shift to the most significant twelve bits
      int16_t correct_freq_value = zcard->freq_tuned[ samples[dac_channel] >> 3];
      spiWrite(spi_channel, (char*) &correct_freq_value, 2);
    }
    else {
      samples_to_dac[0] = channel_map[dac_channel] | (uint16_t) 0;
      samples_to_dac[1] = 0;
      spiWrite(spi_channel, samples_to_dac, 2);
    }

    zcard->previous_samples[dac_channel] = samples[dac_channel]; // use provided value, not mapped value, for cache
  }


  for (; dac_channel < NUM_DAC_CHANNELS; ++dac_channel) {
    if (zcard->previous_samples[dac_channel] != samples[dac_channel] ) {
      // DAC write:
      // bits 15-0:
      // 0 A2 A1 A0 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
      // MSB zero specifies DAC data.  Next three bits are DAC address.  Final 12 are data.
      // Given a 16-bit signed input, write it to a 12-bit signed values.
      // Any negative value clips to zero.

      // Special handling for triangle out to slew rate limit.  Limit is
      // approx 2.5 ms for full off -- full on or vice versa.
      if (dac_channel == triangle_vca_id) {
        int16_t triangle_sample;
        int slew_delta = samples[dac_channel] - zcard->previous_samples[dac_channel];
        if (slew_delta > slew_limit) { // limit positive
          triangle_sample = zcard->previous_samples[dac_channel] + slew_limit;
        }
        else if (slew_delta < -slew_limit) {
          triangle_sample = zcard->previous_samples[dac_channel] - slew_limit;
        }
        else {
          triangle_sample = samples[dac_channel];
        }

        if (triangle_sample >= 0) {
          samples_to_dac[0] = channel_map[dac_channel] | ((uint16_t) triangle_sample) >> 11;
          samples_to_dac[1] = ((uint16_t) triangle_sample) >> 3;
          zcard->previous_samples[dac_channel] = triangle_sample;
        }
        else {
          samples_to_dac[0] = channel_map[dac_channel] | (uint16_t) 0;
          samples_to_dac[1] = 0;
          zcard->previous_samples[dac_channel] = 0;
        }
      }
      else {
        if (samples[dac_channel] >= 0) {
          samples_to_dac[0] = channel_map[dac_channel] | ((uint16_t) samples[dac_channel]) >> 11;
          samples_to_dac[1] = ((uint16_t) samples[dac_channel]) >> 3;
          zcard->previous_samples[dac_channel] = samples[dac_channel];
        }
        else {
          samples_to_dac[0] = channel_map[dac_channel] | (uint16_t) 0;
          samples_to_dac[1] = 0;
          zcard->previous_samples[dac_channel] = 0;
        }
      }

      spiWrite(spi_channel, samples_to_dac, 2);
    }
  }

  return 0;
}


int process_midi(void *zcard_plugin, uint8_t *midi_message, size_t size) {
  return 0;
}


struct midi_program_to_gpio {
  int port;
  uint8_t gpio_reg; // gpio register zero or one?
  uint8_t set_bits;  // mask to OR
  uint8_t clear_bits; // mask to AND
};

// array indexed by MIDI program number
static const struct midi_program_to_gpio midi_program_to_gpio[] = {
  { 0, port0_addr, 0b00000000, 0b11101111 }, // prog 0 - sync neg off
  { 0, port0_addr, 0b00010000, 0b11111111 }, // prog 1 - sync neg on
  { 1, port1_addr, 0b00000000, 0b11111101 }, // prog 2 - mix1 pulse off
  { 1, port1_addr, 0b00000010, 0b11111111 }, // prog 3 - mix1 pulse on
  { 1, port1_addr, 0b00000000, 0b11111110 }, // prog 4 - mix1 comp off
  { 1, port1_addr, 0b00000001, 0b11111111 }, // prog 5 - mix1 comp on
  { 1, port1_addr, 0b00000000, 0b10111111 }, // prog 6 - mix2 pulse off
  { 1, port1_addr, 0b01000000, 0b11111111 }, // prog 7 - mix2 pulse on
  { 1, port1_addr, 0b00000000, 0b01111111 }, // prog 8 - ext mod pwm off
  { 1, port1_addr, 0b10000000, 0b11111111 }, // prog 9 - ext mod pwm on
  { 0, port0_addr, 0b00000000, 0b11011111 }, // prog 10 - ext mod to fm off
  { 0, port0_addr, 0b00100000, 0b11111111 }, // prog 11 - ext mod to fm on
  { 0, port0_addr, 0b00000000, 0b01111111 }, // prog 12 - linear fm off
  { 0, port0_addr, 0b10000000, 0b11111111 }, // prog 13 - linear fm on
  { 1, port1_addr, 0b00000000, 0b11101111 }, // prog 14 - mix2 saw off
  { 1, port1_addr, 0b00010000, 0b11111111 }, // prog 15 - mix2 saw on
  { 0, port0_addr, 0b00000000, 0b10111111 }, // prog 16 - sync pos off
  { 0, port0_addr, 0b01000000, 0b11111111 }, // prog 17 - sync pos on
  { 1, port1_addr, 0b00000000, 0b11110011 }, // prog 18 - mix1 saw off
  { 1, port1_addr, 0b00001000, 0b11111011 }, // prog 19 - mix1 saw low
  { 1, port1_addr, 0b00000100, 0b11110111 }, // prog 20 - mix1 saw med
  { 1, port1_addr, 0b00001100, 0b11111111 }, // prog 21 - mix1 saw high
  { 0, port0_addr, 0b00000000, 0b11111000 }, // prog 22 - ext select card1 out1
  { 0, port0_addr, 0b00000100, 0b11111100 }, // prog 23 - ext select card1 out2
  { 0, port0_addr, 0b00000010, 0b11111010 }, // prog 24 - ext select card2 out1
  { 0, port0_addr, 0b00000110, 0b11111110 }, // prog 25 - ext select card3 out1
  { 0, port0_addr, 0b00000001, 0b11111001 }, // prog 26 - ext select card4 out1
  { 0, port0_addr, 0b00000101, 0b11111101 }, // prog 27 - ext select card5 out1
  { 0, port0_addr, 0b00000011, 0b11111011 }, // prog 28 - ext select card6 out1
  { 0, port0_addr, 0b00000111, 0b11111111 }  // prog 29 - ext select card7 out1
};

int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;

  INFO("Z3340: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

  }
  else {
    WARN("Z3340: unexpected midi program number: 0x%X", program_number);
  }

  return error;
}


//
// Tuning!
//

/** tunereq_save_state
 * Save current state of gpio registers.
 * The DAC values are already stored as previous_samples, should we
 * want to restore them.
 */
int tunereq_save_state(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  int error, spi_channel;

  // prep for tune
  zcard->tuning_point = 0;
  zcard->tuning_complete = 0;
  memset(zcard->tuning_points, 0, sizeof(struct tuning_measurement) * NUM_TUNING_POINTS);

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);
  // set DAC state
  for (int i = 0; i < NUM_DAC_CHANNELS; ++i) {
    spiWrite(spi_channel, (char*)tune_dac_state[i], 2);
  }


  // set any state necessary on the gpio -- all modulations off, outputs off
  error = i2cWriteByteData(zcard->i2c_handle, port0_addr, tune_gpio_port0_data);
  error += i2cWriteByteData(zcard->i2c_handle, port1_addr, tune_gpio_port1_data);
  if (error) {
    ERROR("z3340: tunereq_save_state: error writing to I2C bus handle %d\n", zcard->i2c_handle);
    return TUNE_COMPLETE_FAILED;
  }


  return TUNE_CONTINUE;
}



/** tunereq_set_point
 * set the next tuning point.  "Next" state is tracked by zcard->tuning_point.
 */
const static uint8_t led_bit = 0x8;

tune_status_t tunereq_set_point(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  int spi_channel;
  char dac_values[2]; // these are in reverse byte order
  dac_values[0] = tune_freq_dac_values[ zcard->tuning_point ] >> 8;
  dac_values[1] = tune_freq_dac_values[ zcard->tuning_point ];

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);
  spiWrite(spi_channel, dac_values, 2);

  // eye candy- flash LED while tuning
  i2cWriteByteData(zcard->i2c_handle, config_port0_addr,
                   zcard->tuning_point & 0x1 ? tune_gpio_port0_data | led_bit : tune_gpio_port0_data);

  return TUNE_CONTINUE;
}


/** tunereq_measurement
 * record the measurement succeeded, advance state to next tuning point.
 */
tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;

  if (tuning_measurement->samples == 0) {
    ERROR("tuning measure zero samples for tuning point %d",
          zcard->tuning_point);
    return TUNE_COMPLETE_FAILED;
  }
  else if (tuning_measurement->samples < 10) {
    WARN("tuning point %d measured %d samples, few samples for tuning",
         zcard->tuning_point, tuning_measurement->samples);
  }

  zcard->tuning_points[zcard->tuning_point].actual_dac = tune_freq_dac_values[ zcard->tuning_point ];
  zcard->tuning_points[zcard->tuning_point].frequency = tuning_measurement->frequency;
  zcard->tuning_points[zcard->tuning_point].expected_dac =
    octave_delta(tuning_measurement->frequency, tuning_initial_frequency_target) *
    expected_dac_values_per_octave;

  if (++zcard->tuning_point >= NUM_TUNING_POINTS) {
    zcard->tuning_complete = 1;
    return TUNE_COMPLETE_SUCCESS;
  }
  else {
    return TUNE_CONTINUE;
  }
}


/** tunereq_restore_state
 * if tuning measurements succeeded, create correction mapping table.
 * if tuning failed, create linear table (no corrections).
 */

int tunereq_restore_state(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;

  // restore GPIO expander state
  int error = i2cWriteByteData(zcard->i2c_handle, config_port0_addr, zcard->pca9555_port[0]);
  error += i2cWriteByteData(zcard->i2c_handle, config_port1_addr, zcard->pca9555_port[1]);

  if (zcard->tuning_complete) {
    double oct_delta = octave_delta(zcard->tuning_points[1].frequency,
                                    zcard->tuning_points[0].frequency);

    double dac_delta_per_octave = (TWELVE_BITS / (NUM_TUNING_POINTS - 1)) / oct_delta;
    double low_freq_dac_value = dac_value_for_freq(tuning_initial_frequency_target,
                                                   dac_delta_per_octave,
                                                   zcard->tuning_points[0].frequency);
    double slope;

    // overwrite the expected_dac and actual_dac values of the initial entry--
    // basically do a linear interp and move the y-intercept
    zcard->tuning_points[0].expected_dac = 0.0; //user expects dac 0 for tuning_initial_frequency_target
    zcard->tuning_points[0].actual_dac = 0.5 + low_freq_dac_value;

    int tuned_index;
    for (int i = 0; i < NUM_TUNING_POINTS - 1; ++i) {
      for (tuned_index = zcard->tuning_points[i].expected_dac + 0.5;
           tuned_index < zcard->tuning_points[i+1].expected_dac - 0.5 &&
             tuned_index < TWELVE_BITS;
           ++tuned_index) {
        // y = mx + b; we have b as zcard->tuning_points[i].actual_dac
        // compute m and x
        slope = (zcard->tuning_points[i+1].actual_dac - zcard->tuning_points[i].actual_dac) /
          (zcard->tuning_points[i+1].expected_dac - zcard->tuning_points[i].expected_dac);
        int x = tuned_index - (int)(zcard->tuning_points[i].expected_dac + 0.5);
        int y = slope * x + zcard->tuning_points[i].actual_dac;
        zcard->freq_tuned[tuned_index] = y > 0 ? y : 0;
      }
    }

    // fill in remaining elements of table from tuned_index-1 onward.
    INFO("slot %d tuning end point: %d", zcard->slot, tuned_index);
    for (int i = tuned_index - 1; i < TWELVE_BITS; ++i) {
      zcard->freq_tuned[i] = zcard->freq_tuned[tuned_index - 1];
    }

    // cleanup table
    for (int i = 0; i < TWELVE_BITS; ++i) {
      if (zcard->freq_tuned[i] > TWELVE_BITS - 1) {
        zcard->freq_tuned[i] = 0xff0f;
      }
      else {
        unsigned char upper, lower;
        upper = zcard->freq_tuned[i];
        lower = channel_map[0] | (0xf & zcard->freq_tuned[i] >> 8);
        zcard->freq_tuned[i] = ((int16_t)upper) << 8 | (int16_t)lower;
      }
    }


    return TUNE_COMPLETE_SUCCESS;
  }

  // linear table -- no corrections
  create_linear_tuning(channel_map[0], TWELVE_BITS, zcard->freq_tuned);
  return TUNE_COMPLETE_FAILED;
}




/** create_linear_tuning
 * create a linear tuning table - no corrections.  Create it for the passed in DAC channel such that
 * it can be passed straight to spiWrite.
 */
static void create_linear_tuning(int dac_channel, int num_elements, int16_t *table) {
  for (int i = 0; i < num_elements; ++i) {
    unsigned char upper, lower;
    upper = i;
    lower = dac_channel | (0xf & i >> 8);
    table[i] = ((int16_t)upper) << 8 | (int16_t)lower;
  }
}


