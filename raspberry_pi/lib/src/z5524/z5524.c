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

#include "zcard_plugin.h"
#include "tune_utils.h"


// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20

// 2x DAC: Analog Devices AD5328
// https://www.analog.com/media/en/technical-documentation/data-sheets/ad5308_5318_5328.pdf
#define SPI_MODE 1
#define CHIP_SELECTS 2
#define DAC_CHANNELS 8
#define NUM_TUNING_POINTS 9
#define TWELVE_BITS 4096

struct tunable {
  int tuning_point; // maintain state between tuning calls
  struct tune_point tuning_points[NUM_TUNING_POINTS];
  int tuning_complete;
  int16_t freq_tuned[TWELVE_BITS];
};

typedef enum {
  TUNE_SSI2130_VCO,
  TUNE_AS3394_VCO,
  TUNE_AS3394_VCF
} tuning_component_t;

struct z5524_card {
  struct zhost *zhost;
  int slot;
  int i2c_handle;
  uint8_t pca9555_port[2];  // gpio registers
  int16_t previous_samples[CHIP_SELECTS][DAC_CHANNELS];

  // tuning params
  struct tunable as3394_vco;
  struct tunable as3394_vcf;
  struct tunable ssi2130_vco;
  tuning_component_t tune_component;
};

static const uint8_t port0_addr = 0x02;
static const uint8_t port1_addr = 0x03;
static const uint8_t config_port0_addr = 0x06;
static const uint8_t config_port1_addr = 0x07;
static const uint8_t config_port_as_output = 0x00;
static const uint8_t startup_hard_sync_value = 0xC2;
static const uint16_t startup_as3394_high_freq = 0x7FFF;
static const uint16_t startup_as3394_vco_dac_line = 6;


// channel for DAC is upper 4 bits
static const int channel_map[] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 };

// slew rate limit these signals
static const int final_vca_cs = 0;
static const int final_vca_dac = 2;
static const int slew_limit = 2000; // ~4ms max rise or fall time: 250us * 32768/2000

// helper declarations
inline static int dac_write(int16_t this_sample, int dac_line, int spi_channel);



static void create_linear_tuning(int dac_channel, int num_elements, int16_t *table);



/* init_zcard
 * set gpio & dac initial values.  Of note: the SSI2130 doesn't always
 * startup unless a hard sync signal is sent.  To do this, enable hard
 * and crank frequency to a high level.  Enough to make sure a sync
 * pulse or two is sent with a short sleep.
 */
void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  int spi_channel;
  // AD5328: control register
  char dac_ctrl0_reg[2] = { 0b11110000, 0b00000000 };  // full reset, data and control
  char dac_ctrl1_reg[2] = { 0b10000000, 0b00000000 };  // power on, gain 1x, unbuffered Vref input

  assert(slot >= 0 && slot < 8);
  struct z5524_card *z5524 = (struct z5524_card*)calloc(1, sizeof(struct z5524_card));
  if (z5524 == NULL) {
    return NULL;
  }

  z5524->zhost = zhost;
  z5524->slot = slot;
  z5524->pca9555_port[0] = 0x00;
  z5524->pca9555_port[1] = startup_hard_sync_value; // hard sync enabled

  z5524->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (z5524->i2c_handle < 0) {
    ERROR("z5524: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(z5524->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(z5524->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(z5524->i2c_handle, port0_addr, z5524->pca9555_port[0]);
  error += i2cWriteByteData(z5524->i2c_handle, port1_addr, z5524->pca9555_port[1]);

  if (error) {
    ERROR("z5524: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(z5524->i2c_handle);
    free(z5524);
    return NULL;
  }

  // init previous_samples to non-valid value
  for (int i = 0; i < CHIP_SELECTS; ++i) {
    for (int j = 0; j < DAC_CHANNELS; ++j) {
      z5524->previous_samples[i][j] = -1;
    }
  }


  // configure DAC
  spi_channel = set_spi_interface(zhost, 0, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);
  dac_write(startup_as3394_high_freq, startup_as3394_vco_dac_line, spi_channel);

  spi_channel = set_spi_interface(zhost, 1, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);



  return z5524;
}


void free_zcard(void *zcard_plugin) {
  struct z5524_card *z5524 = (struct z5524_card*)zcard_plugin;

  if (zcard_plugin) {
    if (z5524->i2c_handle >= 0) {
      // TODO: turn off LED
      i2cClose(z5524->i2c_handle);
    }
    free(z5524);
  }
}

char* get_plugin_name() {
  return "Zoxnoxious 5524";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = 16;
  props->spi_mode = 0;
  return props;
}



int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;
  int spi_channel;
  int16_t this_sample;
  int slew_delta;
  int chip_select = 0;

  // dac output samples
  spi_channel = set_spi_interface(zcard->zhost, chip_select, SPI_MODE, zcard->slot);

  for (int i = 0; i < DAC_CHANNELS; ++i) {
    this_sample = samples[i + chip_select * DAC_CHANNELS];
    if (zcard->previous_samples[chip_select][i] != this_sample) {

      // special handling for final_vca with a slew rate limit filter
      // The 3394 final_vca should be slew rate limited.  From zero to max
      // should be a couple milliseconds, not 250us.  Avoid clicks.
      if (i == final_vca_dac) {
        slew_delta = this_sample - zcard->previous_samples[final_vca_cs][final_vca_dac];
        if (slew_delta > slew_limit) { // limit positive
          this_sample = zcard->previous_samples[final_vca_cs][final_vca_dac] + slew_limit;
        }
        else if (slew_delta < -slew_limit) {
          this_sample = zcard->previous_samples[final_vca_cs][final_vca_dac] - slew_limit;
        }
      }

      zcard->previous_samples[chip_select][i] = this_sample;
      dac_write(this_sample, channel_map[i], spi_channel);
    }
  }

  chip_select++;
  spi_channel = set_spi_interface(zcard->zhost, chip_select, SPI_MODE, zcard->slot);

  // and the DAC out for CS1, the SSI2130.  Yes, this could be a loop for chip
  // select but this pulls the check for 3394 final_vca_dac out.  No need to
  // check that condition here.
  for (int i = 0; i < DAC_CHANNELS; ++i) {
    this_sample = samples[i + chip_select * DAC_CHANNELS];
    if (zcard->previous_samples[chip_select][i] != this_sample) {
      zcard->previous_samples[chip_select][i] = this_sample;
      dac_write(this_sample, channel_map[i], spi_channel);
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

// VCO1 == SSI2130
// VCO2 == AS3394

// array indexed by MIDI program number -- maybe todo: reorder to match wiring
static const struct midi_program_to_gpio midi_program_to_gpio[] = {
  { 1, port1_addr, 0b11000000, 0b11111111 }, // prog 0 - VCO2 Saw Off Tri Off
  { 1, port1_addr, 0b01000000, 0b01111111 }, // prog 1 - VCO2 Saw Off Tri On
  { 1, port1_addr, 0b00000000, 0b00111111 }, // prog 2 - VCO2 Saw On  Tri Off
  { 1, port1_addr, 0b10000000, 0b10111111 }, // prog 3 - VCO2 Saw On  Tri On
  { 1, port1_addr, 0b00000000, 0b11101111 }, // prog 4 - VCO1 Exp FM Off
  { 1, port1_addr, 0b00010000, 0b11111111 }, // prog 5 - VCO1 Exp FM On
  { 1, port1_addr, 0b00000000, 0b11011111 }, // prog 6 - VCO1 to Wave Select Off
  { 1, port1_addr, 0b00100000, 0b11111111 }, // prog 7 - VCO1 to Wave Select On
  { 1, port1_addr, 0b00000000, 0b11110111 }, // prog 8 - VCO2 to VCO1 FM Freq Off
  { 1, port1_addr, 0b00001000, 0b11111111 }, // prog 9 - VCO2 to VCO1 FM Freq On
  { 1, port1_addr, 0b00000000, 0b11111011 }, // prog 10 - VCO2 to Soft Sync Off
  { 1, port1_addr, 0b00000100, 0b11111111 }, // prog 11 - VCO2 to Soft Sync On

  { 0, port0_addr, 0b00000000, 0b11101111 }, // prog 12 - VCO1 to VCO2 PW Off
  { 0, port0_addr, 0b00010000, 0b11111111 }, // prog 13 - VCO1 to VCO2 PW On
  { 0, port0_addr, 0b00000000, 0b11011111 }, // prog 14 - VCO1 to VCF Off
  { 0, port0_addr, 0b00100000, 0b11111111 }, // prog 15 - VCO1 to VCF On
  { 1, port1_addr, 0b00000000, 0b11111110 }, // prog 16 - VCO2 to VCO1 PW Off
  { 1, port1_addr, 0b00000001, 0b11111111 }, // prog 17 - VCO2 to VCO1 PW On
  { 1, port1_addr, 0b00000000, 0b11111101 }, // prog 18 - VCO2 to VCO1 Sub Hard Sync Off
  { 1, port1_addr, 0b00000010, 0b11111111 }  // prog 19 - VCO2 to VCO1 Sub Hard Sync On
};

int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;

  INFO("Z5524: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

  }
  else {
    WARN("Z5524: unexpected midi program number: 0x%X", program_number);
  }

  return error;
}


// DAC write:
// bits 15-0:
// 0 A2 A1 A0 D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
// MSB zero specifies DAC data.  Next three bits are DAC address.  Final 12 are data.
// Given a 16-bit signed input, write it to a 12-bit signed values.
// Any negative value clips to zero.
inline static int dac_write(int16_t this_sample, int dac_line, int spi_channel) {
  char samples_to_dac[2];

  if (this_sample >= 0) {
    samples_to_dac[0] = dac_line | (this_sample) >> 11;
    samples_to_dac[1] = this_sample >> 3;
  }
  else {
    samples_to_dac[0] = dac_line | (uint16_t) 0;
    samples_to_dac[1] = (uint16_t) 0;
  }

  return spiWrite(spi_channel, samples_to_dac, 2);
}



//
// Tuning section
// Put all the tuning stuff down here.  A lot of it.
// This tunes the VCOs on both chips and the VCF to 1 volt/octave.
//


static const uint8_t tune_config_port0_data = 0x00;
static const uint8_t tune_config_port1_data = 0b11000000; // wave select: saw/tri off
// tune_dac_state is in spiWrite order
// set these values when tuning the 2130
static const char tune_2130_dac_state_as3394[][2] = { { 0x0f, 0xff }, // vco mix: 2130
                                                      { 0x10, 0x00 }, // pwm: off
                                                      { 0x2f, 0xff }, // final gain
                                                      { 0x30, 0x00 }, // vcf mod
                                                      { 0x40, 0x00 }, // resonance
                                                      { 0x50, 0x00 }, // 2130 mod vca
                                                      { 0x60, 0x00 }, // 3394 vco exp freq
                                                      { 0x7f, 0xff } }; // vcf: open

static const char tune_2130_dac_state_ssi2130[][2] = { { 0x00, 0x00 }, // tri mix
                                                       { 0x17, 0xff }, // linear: mid-range
                                                       { 0x20, 0x00 }, // exp pitch
                                                       { 0x37, 0xff }, // pwm
                                                       { 0x40, 0x00 }, // sine vca
                                                       { 0x50, 0x00 }, // 3394 mod vca
                                                       { 0x6f, 0xff }, // pulse mix: 100%
                                                       { 0x70, 0x00 } }; // saw mix

// set these values when tuning the 3394 vco
static const char tune_3394_vco_dac_state_as3394[][2] = { { 0x00, 0x00 }, // vco mix: 3394
                                                          { 0x17, 0xff }, // pwm: 50%
                                                          { 0x2f, 0xff }, // final gain
                                                          { 0x30, 0x00 }, // vcf mod
                                                          { 0x40, 0x00 }, // resonance
                                                          { 0x50, 0x00 }, // 2130 mod vca
                                                          { 0x60, 0x00 }, // 3394 vco exp freq
                                                          { 0x7f, 0xff } }; // vcf: open

static const char tune_3394_dac_state_ssi2130[][2] = { { 0x00, 0x00 }, // tri mix
                                                       { 0x17, 0xff }, // linear: mid-range
                                                       { 0x20, 0x00 }, // exp pitch
                                                       { 0x37, 0xff }, // pwm
                                                       { 0x40, 0x00 }, // sine vca
                                                       { 0x50, 0x00 }, // 3394 mod vca
                                                       { 0x60, 0x00 }, // pulse mix: off
                                                       { 0x70, 0x00 } }; // saw mix

// set these values when tuning the 3394 vcf (ssi2130 as per above)
static const char tune_3394_vcf_dac_state_as3394[][2] = { { 0x00, 0x00 }, // vco mix: 3394
                                                          { 0x10, 0x00 }, // pwm: 0%
                                                          { 0x2f, 0xff }, // final gain
                                                          { 0x30, 0x00 }, // vcf mod
                                                          { 0x4f, 0xff }, // res: crank it!
                                                          { 0x50, 0x00 }, // 2130 mod vca
                                                          { 0x60, 0x00 }, // 3394 vco exp freq
                                                          { 0x7f, 0xff } }; // vcf: open


// tune frequency dac values: the DAC values to run through for a tuning request.
// Revese the byte order before calling spiWrite and logical or with DAC line.
// These values are used for both the 2130 and 3394.
static const int16_t tune_freq_dac_values[] = { 0x0000,
                                                0x0200,
                                                0x0400,
                                                0x0600,
                                                0x0800,
                                                0x0a00,
                                                0x0c00,
                                                0x0e00,
                                                0x0fff };

static const double tuning_initial_frequency_target = 27.5;
static const double expected_dac_values_per_octave = 512; // for 12 bits / 8 octave range
static const double expected_dac_vcf_values_per_octave = 409.6; // for 12 bits / 10 octave range


/** tunereq_save_state
 * The DAC and gpio can be restored based on existing state, so no need to save those.
 * Setup tuning structures
 */
int tunereq_save_state(void *zcard_plugin) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;
  int error, spi_channel;

  // structure prep
  memset(&zcard->as3394_vco, 0, sizeof(struct tunable));
  memset(&zcard->as3394_vcf, 0, sizeof(struct tunable));
  memset(&zcard->ssi2130_vco, 0, sizeof(struct tunable));
  zcard->tune_component = TUNE_SSI2130_VCO; // start with the 2130

  // set DAC values for SSI2130
  int chip_select = 0;
  spi_channel = set_spi_interface(zcard->zhost, chip_select, SPI_MODE, zcard->slot);
  // set DAC state AS3394
  for (int i = 0; i < DAC_CHANNELS; ++i) {
    spiWrite(spi_channel, (char*)tune_2130_dac_state_as3394[i], 2);
  }
  spi_channel = set_spi_interface(zcard->zhost, ++chip_select, SPI_MODE, zcard->slot);
  // set DAC state SSI2130
  for (int i = 0; i < DAC_CHANNELS; ++i) {
    spiWrite(spi_channel, (char*)tune_2130_dac_state_ssi2130[i], 2);
  }


  // set any state necessary on the gpio -- all modulations off, outputs set
  error = i2cWriteByteData(zcard->i2c_handle, config_port0_addr, tune_config_port0_data);
  error += i2cWriteByteData(zcard->i2c_handle, config_port1_addr, tune_config_port1_data);
  if (error) {
    ERROR("z5524: tunereq_save_state: error writing to I2C bus handle %d\n", zcard->i2c_handle);
    return TUNE_COMPLETE_FAILED;
  }

  return TUNE_CONTINUE;
}



tune_status_t tunereq_set_point(void *zcard_plugin) {
  return TUNE_COMPLETE_SUCCESS;
}
tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  return TUNE_COMPLETE_SUCCESS;
}
int tunereq_restore_state(void *zcard_plugin) {
  struct z5524_card *zcard = (struct z5524_card*)zcard_plugin;

  // restore gpio
  int error = i2cWriteByteData(zcard->i2c_handle, config_port0_addr, zcard->pca9555_port[0]);
  error += i2cWriteByteData(zcard->i2c_handle, config_port1_addr, zcard->pca9555_port[1]);

  // restore DAC (or set to invalid state)

  return TUNE_COMPLETE_SUCCESS;
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


