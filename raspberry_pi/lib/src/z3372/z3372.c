/* Copyright 2024 Kyle Farrell
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

#include "zcard_plugin.h"


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20


const uint8_t port0_init = 0x00; // port0: all off, active low LED on
const uint8_t port1_init= 0x00; // port1: all off, select muxes disabled
const uint8_t port0_addr = 0x02;
const uint8_t port1_addr = 0x03;
const uint8_t config_port0_addr = 0x06;
const uint8_t config_port1_addr = 0x07;
const uint8_t config_port_as_output = 0x00;

void create_linear_tuning(int dac_channel, int num_elements, int16_t *table);


// seven audio channels mapped
// signal / audio channel / dac channel
// Noise level / 0 / 0x00
// Pan / 1 / 0x01
// Resonance Level / 2 / 0x02
// Output VCA / 3 / 0x03
// VCF cutoff CV / 4 / 0x04
// Source One Level / 5 / 0x05
// Source Two Level / 6 / 0x06
// Mod Amount / 7 / 0x07
// DAC channel is the upper 4 bits of the SPI byte, so set the map up as such
static const int cutoff_cv_dac = 4;
static const uint8_t channel_map[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 };


void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  int spi_channel;
  // AD5328: control register
  char dac_ctrl0_reg[2] = { 0b11110000, 0b00000000 };  // full reset, data and control
  char dac_ctrl1_reg[2] = { 0b10000000, 0b00110000 };  // power on, gain 2x, unbuffered Vref input

  assert(slot >= 0 && slot < 8);
  struct z3372_card *z3372 = (struct z3372_card*)calloc(1, sizeof(struct z3372_card));
  if (z3372 == NULL) {
    return NULL;
  }

  z3372->zhost = zhost;
  z3372->slot = slot;
  z3372->pca9555_port[0] = port0_init;
  z3372->pca9555_port[1] = port1_init;

  z3372->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (z3372->i2c_handle < 0) {
    ERROR("z3372: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(z3372->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(z3372->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(z3372->i2c_handle, port0_addr, z3372->pca9555_port[0]);
  error += i2cWriteByteData(z3372->i2c_handle, port1_addr, z3372->pca9555_port[1]);

  if (error) {
    ERROR("z3372: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(z3372->i2c_handle);
    free(z3372);
    return NULL;
  }

  // configure DAC
  spi_channel = set_spi_interface(zhost, SPI_CHANNEL, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);

  for (int i = 0; i < NUM_CHANNELS; ++i) {
    z3372->previous_samples[i] = -1;
  }


  z3372->tunable.dac_size = TWELVE_BITS;
  z3372->tunable.dac_calibration_table = (int16_t*)calloc(TWELVE_BITS, sizeof(int16_t));
  z3372->tunable.tune_points = (struct tune_point*)calloc(NUM_TUNING_POINTS, sizeof(struct tune_point));
  z3372->tunable.tune_points_size = NUM_TUNING_POINTS;

  // on init use a linear tuning.  Autotune not in effect.
  create_linear_tuning(filter_dac_channel,
                       z3372->tunable.dac_size,
                       z3372->tunable.dac_calibration_table);

  return z3372;
}


void free_zcard(void *zcard_plugin) {
  struct z3372_card *z3372 = (struct z3372_card*)zcard_plugin;

  if (zcard_plugin) {
    free(z3372->tuneable.dac_calibration_table);
    free(z3372->tuneable.tune_points);

    if (z3372->i2c_handle >= 0) {
      // TODO: turn off LED
      i2cClose(z3372->i2c_handle);
    }
    free(z3372);
  }
}

char* get_plugin_name() {
  return "Zoxnoxious 3372 VCF";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = NUM_CHANNELS;
  props->spi_mode = 0;
  return props;
}



int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct z3372_card *zcard = (struct z3372_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);

  for (int i = 0; i < NUM_CHANNELS; ++i) {
    if (zcard->previous_samples[i] != samples[i] ) {

      // DAC write:
      // bits 15-0:
      // 0 A2 A1 A0 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
      // MSB zero specifies DAC data.  Next three bits are DAC address.  Final 12 are data.
      // Given a 16-bit signed input, write it to a 12-bit signed values.
      // Any negative value clips to zero.
      if (samples[i] >= 0) {
        zcard->previous_samples[i] = samples[i];
        if (i == cutof_cv_dac) {
          spiWrite(spi_channel, (char*) &zcard->tunable.dac_calibration_table[ samples[i] ], 2);
        }
        else {
          samples_to_dac[0] = channel_map[i] | ((uint16_t) samples[i]) >> 11;
          samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;
          spiWrite(spi_channel, samples_to_dac, 2);
        }
      }
      else {
        zcard->previous_samples[i] = 0;
        samples_to_dac[0] = channel_map[i] | (uint16_t) 0;
        samples_to_dac[1] = 0;
        spiWrite(spi_channel, samples_to_dac, 2);
      }

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
  { 0, port0_addr, 0b00000000, 0b11111011 }, // prog 0 - filter fm off
  { 0, port0_addr, 0b00000100, 0b11111111 }, // prog 1 - filter fm on
  { 0, port0_addr, 0b00000000, 0b11101111 }, // prog 2 - vca mod off
  { 0, port0_addr, 0b00010000, 0b11111111 }, // prog 3 - vca mod on

  { 1, port1_addr, 0b00001111, 0b11111111 }, // prog 4 - card 1/ out1
  { 1, port1_addr, 0b00001101, 0b11111101 }, // prog 5 - 1/2
  { 1, port1_addr, 0b00001011, 0b11111011 }, // prog 6 - 2/2
  { 1, port1_addr, 0b00001001, 0b11111001 }, // prog 7 - 3/1
  { 1, port1_addr, 0b00000111, 0b11110111 }, // prog 8 - 4/2
  { 1, port1_addr, 0b00000101, 0b11110101 }, // prog 9 - 5/1
  { 1, port1_addr, 0b00000011, 0b11110011 }, // prog 10 - 6/2
  { 1, port1_addr, 0b00000001, 0b11110001 }, // prog 11 - 7/1

  { 1, port1_addr, 0b11110000, 0b11111111 }, // prog 12 - card 1 / out 1
  { 1, port1_addr, 0b11010000, 0b11011111 }, // prog 13 - 1/2
  { 1, port1_addr, 0b10110000, 0b10111111 }, // prog 14 - 2/1
  { 1, port1_addr, 0b10010000, 0b10011111 }, // prog 15 - 2/2
  { 1, port1_addr, 0b01110000, 0b01111111 }, // prog 16 - 3/2
  { 1, port1_addr, 0b01010000, 0b01011111 }, // prog 17 - 4/1
  { 1, port1_addr, 0b00110000, 0b00111111 }, // prog 18 - 5/2
  { 1, port1_addr, 0b00010000, 0b00011111 }, // prog 19 - 6/1

  { 0, port0_addr, 0b00000000, 0b11110111 }, // prog 20 - rez mod off
  { 0, port0_addr, 0b00001000, 0b11111111 }, // prog 21 - rez mod on
  { 0, port0_addr, 0b00000000, 0b11011111 }, // prog 22 - panning modulation off
  { 0, port0_addr, 0b00100000, 0b11111111 }  // prog 23 - panning modulation on

};


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct z3372_card *zcard = (struct z3372_card*)zcard_plugin;

  INFO("Z3372: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

  }
  else {
    WARN("Z3372: unexpected midi program number: 0x%X", program_number);
  }

  return error;
}



tune_status_t tunereq_save_state(void *zcard_plugin) {
  return TUNE_COMPLETE_SUCCESS;
}
tune_status_t tunereq_set_point(void *zcard_plugin) {
  return TUNE_COMPLETE_SUCCESS;
}
tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  return TUNE_COMPLETE_SUCCESS;
}
tune_status_t tunereq_restore_state(void *zcard_plugin) {
  return TUNE_COMPLETE_SUCCESS;
}


/** create_linear_tuning
 * create a linear tuning table - no corrections.  Create it for the passed in DAC channel such that
 * it can be passed straight to spiWrite.
 */
void create_linear_tuning(int dac_channel, int num_elements, int16_t *table) {
  for (int i = 0; i < num_elements; ++i) {
    unsigned char upper, lower;
    upper = i;
    lower = dac_channel | (0xf & i >> 8);
    table[i] = ((int16_t)upper) << 8 | (int16_t)lower;
  }
}
