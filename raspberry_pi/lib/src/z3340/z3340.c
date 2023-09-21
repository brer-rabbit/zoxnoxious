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


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20

// DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1
#define SPI_CHANNEL 0 // chip select zero
#define NUM_DAC_CHANNELS 6


struct z3340_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples[NUM_DAC_CHANNELS];

  uint8_t tune_store_pca9555_port[2];
  float frequency_tuning_points[9];
  int tuning_num_samples;
  int freq_tuning_index;  // for loop index to frequency_tuning_points
  uint32_t gpio_mask;
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
static const uint8_t tune_config_port0_data = 0x00;
static const uint8_t tune_config_port1_data = 0x02; // output1 pulse -- not actually necessary
static char tune_dac_state[][2] = { { 0x00, 0x00 }, // freq
                                    { 0x10, 0x00 }, // sync level
                                    { 0x38, 0x00 }, // pulse width: 50%
                                    { 0x40, 0x00 }, // tri vca
                                    { 0x50, 0x00 }, // ext mod
                                    { 0x78, 0x00 } }; // linear: mid-range
// tune_freq_dac_values: the DAC values to run through for a tuning request:
// all on frequency CV, each step is up by 1 volt (512 decimal).
static char tune_freq_dac_values[][2] = { { 0x00, 0x00 }, // freq 0V
                                                { 0x02, 0x00 }, // freq 1V
                                                { 0x04, 0x00 }, // freq 2V
                                                { 0x06, 0x00 }, // freq 3V
                                                { 0x08, 0x00 }, // freq 4V
                                                { 0x0a, 0x00 }, // freq 5V
                                                { 0x0c, 0x00 }, // freq 6V
                                                { 0x0e, 0x00 }, // freq 7V
                                                { 0x0f, 0xff } }; // freq 8V
static void read_samples(const gpioSample_t *samples, int numSamples, void *userdata);


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

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);

  // this is broken up, rather crudely, to filter the triangle VCA amount.  If not
  // for that a single loop would do
  // first channels: freq_cv, sync_level, pwm
  for (int dac_channel = 0; dac_channel < NUM_DAC_CHANNELS; ++dac_channel) {
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



/** tunereq_save_state
 * Save current state of gpio registers.
 * The DAC values are already stored as previous_samples.
 */
int tunereq_save_state(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;

  INFO("Z3340: tune request save state");
  zcard->tune_store_pca9555_port[0] = zcard->pca9555_port[0];
  zcard->tune_store_pca9555_port[1] = zcard->pca9555_port[1];


  return 0;
}



// TODO: move this to the zdk lib
static const int gpio_id_by_slot[] = { 17, 27, 22, 23, 24, 25 };

int tunereq_tune_card(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;
  int error;
  int spi_channel;

  zcard->gpio_mask = 1 << gpio_id_by_slot[zcard->slot]; // get gpio number of slot, set mask bit
  INFO("Z3340: tune request tune card using slot %d gpio mask %u", zcard->slot, zcard->gpio_mask);

  // set any state necessary on the gpio -- all modulations off, outputs off
  error = i2cWriteByteData(zcard->i2c_handle, config_port0_addr, tune_config_port0_data);
  error += i2cWriteByteData(zcard->i2c_handle, config_port1_addr, tune_config_port1_data);
  if (error) {
    ERROR("z3340: error writing to I2C bus handle %d\n", zcard->i2c_handle);
    return -1;
  }

  spi_channel = set_spi_interface(zcard->zhost, SPI_CHANNEL, SPI_MODE, zcard->slot);
  // set DAC state
  for (int i = 0; i < NUM_DAC_CHANNELS; ++i) {
    spiWrite(spi_channel, tune_dac_state[i], 2);
  }

  INFO("Z3340: tune request slot %d setup done", zcard->slot);

  // ready to setup frequency measurement
  for (zcard->freq_tuning_index = 0;
       zcard->freq_tuning_index < sizeof(tune_freq_dac_values) / sizeof(tune_freq_dac_values[0]);
       ++zcard->freq_tuning_index) {
    // set DAC
    spiWrite(spi_channel, tune_freq_dac_values[zcard->freq_tuning_index], 2);

    zcard->frequency_tuning_points[zcard->freq_tuning_index] = 0.0;
    zcard->tuning_num_samples = 0;
    // register callback
    gpioSetGetSamplesFuncEx(read_samples, zcard->gpio_mask, zcard);

    // usleep 100ms
    usleep(100000);

    // unregister callback
    gpioSetGetSamplesFuncEx(NULL, 0, zcard);
    INFO("Z3340: slot %d tune %i done: %f / %d",
         zcard->slot,
         zcard->freq_tuning_index,
         zcard->frequency_tuning_points[zcard->freq_tuning_index],
         zcard->tuning_num_samples);
  }

  INFO("Z3340: tune slot %d complete", zcard->slot);

  return 0;
}




int tunereq_restore_state(void *zcard_plugin) {
  struct z3340_card *zcard = (struct z3340_card*)zcard_plugin;


  INFO("Z3340: tune request restore state");
  zcard->pca9555_port[0] = zcard->tune_store_pca9555_port[0];
  zcard->pca9555_port[1] = zcard->tune_store_pca9555_port[1];

  return 0;
}



// internal / static functions

// Callback function to implement gpioGetSamplesFuncEx_t.  Read the
// samples from gpioGetSamplesFuncEx.  Store the results with the
// z3340_card userdata.

static void read_samples(const gpioSample_t *samples, int num_samples, void *userdata) {
  struct z3340_card *zcard = (struct z3340_card*)userdata;
  int sample_index;
  int gpio_low_to_high;

  for (sample_index = 1; sample_index < num_samples; ++sample_index) {
    // xor to find any changes, and with gpio mask to get bit of interest
    gpio_low_to_high =
      (samples[sample_index - 1].level ^ samples[sample_index].level) & zcard->gpio_mask;

    // todo: record last low to high for delta
    if (gpio_low_to_high) { // if it's a low to high
      zcard->tuning_num_samples++;
      zcard->frequency_tuning_points[zcard->freq_tuning_index] +=
        (samples[sample_index].tick - samples[sample_index - 1].tick);
    }
  }

}
