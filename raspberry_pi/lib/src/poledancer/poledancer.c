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
#include "poledancer.h"

// GPIO: PCA9555
#define PCA9555_BASE_I2C_ADDRESS 0x20
#define EEPROM_BASE_I2C_ADDRESS 0x50


const uint8_t port0_init = 0x00; // port0: select muxes disabled
const uint8_t port1_init= 0x00; // port1: all switches off, active low LED on
const uint8_t port0_addr = 0x02;
const uint8_t port1_addr = 0x03;
const uint8_t config_port0_addr = 0x06;
const uint8_t config_port1_addr = 0x07;
const uint8_t config_port_as_output = 0x00;

const int spi_channel_cs0 = 0;
const int spi_channel_cs1 = 1;

// dac lines to use for chip select zero, one
// cs : dac addr : audio channel : signal
// 0 : 0x00 : 0 : vcf cutoff
// 0 : 0x40 : 1 : source1 audio vca
// 0 : 0x50 : 2 : source2 audio vca
// 0 : 0x60 : 3 : source1 mod vca
// 0 : 0x70 : 4 : source2 mod vca
// 1 : 0x00 : N/A : ctrl ref (calibration - not mapped to an audio channel)
// 1 : 0x10 : 9 : pole4 level
// 1 : 0x20 : 7 : pole2 level
// 1 : 0x30 : 8 : pole3 level
// 1 : 0x40 : 6 : pole1 level
// 1 : 0x70 : 5 : dry level
// 1 : 0x60 : 10 : q vca

static const uint8_t channel_map_cs0[] = { 0x00, 0x40, 0x50, 0x60, 0x70 };
static const uint8_t channel_map_cs1[] = { 0x10, 0x20, 0x30, 0x40, 0x60, 0x70 };
const uint8_t cutoff_cv_channel = 0;

static void create_linear_tuning(int, int, int16_t*);
static void calibrate_vca2190_dac(struct poledancer_card*, int i2c_rom_handle);

// I2C EEPROM layout
// address zero, like all z-cards, has the board identifier.
// In addition to that Pole Dancer also stores DAC calibration values.
// Address | Value
// 0x00    | 0x06 (Pole Dancer identifier)
// 0x01    | 0x01 (Pole Dancer version number)
// 0x02    | Ctrl Ref DAC calib values Min
// 0x04    | Ctrl Ref DAC calib values Max
// 0x06    | Dry VCA DAC calib values Min
// 0x08    | Dry VCA DAC calib values Max
// 0x0A    | Pole1 VCA DAC calib values Min
// 0x0C    | Pole1 VCA DAC calib values Max
// 0x0E    | Pole2 VCA DAC calib values Min
// 0x10    | Pole2 VCA DAC calib values Max
// 0x12    | Pole3 VCA DAC calib values Min
// 0x14    | Pole3 VCA DAC calib values Max
// 0x16    | Pole4 VCA DAC calib values Min
// 0x18    | Pole4 VCA DAC calib values Max
//
static const unsigned int load_rom_address = 0x01;
static const int8_t min_board_version = 1;
#define NUM_CHANNEL_DESCRIPTORS 6
#define ROM_READ_SIZE_BYTES NUM_CHANNEL_DESCRIPTORS * 4 + 1

/* init_zcard
 * set gpio & dac initial values.  
 */

void* init_zcard(struct zhost *zhost, int slot) {
  int error = 0;
  int i2c_addr = slot + PCA9555_BASE_I2C_ADDRESS;
  int spi_channel;
  // AD5328: control register
  char dac_ctrl0_reg[2] = { 0b11110000, 0b00000000 };  // full reset, data and control
  char dac_ctrl1_reg[2] = { 0b10000000, 0b00000000 };  // power on, gain 1x, unbuffered Vref input

  assert(slot >= 0 && slot < 8);
  struct poledancer_card *poledancer = (struct poledancer_card*)calloc(1, sizeof(struct poledancer_card));
  if (poledancer == NULL) {
    return NULL;
  }

  poledancer->zhost = zhost;
  poledancer->slot = slot;
  poledancer->pca9555_port[0] = port0_init;
  poledancer->pca9555_port[1] = port1_init;

  poledancer->i2c_handle = i2cOpen(I2C_BUS, i2c_addr, 0);
  if (poledancer->i2c_handle < 0) {
    ERROR("poledancer: unable to open i2c for address %d\n", i2c_addr);
    return NULL;
  }

  // configure port0 and port1 as output;
  // start with zero values.  This ought to
  // turn everything "off", with the LED on.
  error += i2cWriteByteData(poledancer->i2c_handle, config_port0_addr, config_port_as_output);
  error += i2cWriteByteData(poledancer->i2c_handle, config_port1_addr, config_port_as_output);
  error += i2cWriteByteData(poledancer->i2c_handle, port0_addr, poledancer->pca9555_port[0]);
  error += i2cWriteByteData(poledancer->i2c_handle, port1_addr, poledancer->pca9555_port[1]);

  if (error) {
    ERROR("poledancer: error writing to I2C bus address %d\n", i2c_addr);
    i2cClose(poledancer->i2c_handle);
    free(poledancer);
    return NULL;
  }


  // VCA calibration call - needs ROM i2c handle
  int i2c_rom = i2cOpen(I2C_BUS, slot + EEPROM_BASE_I2C_ADDRESS, 0);
  if (i2c_rom < 0) {
    ERROR("poledancer: unable to open i2c for address %d\n",
          slot + EEPROM_BASE_I2C_ADDRESS);
    return NULL;
  }
  calibrate_vca2190_dac(poledancer, i2c_rom);

  i2cClose(i2c_rom);


  // configure DAC
  spi_channel = set_spi_interface(zhost, spi_channel_cs0, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);

  spi_channel = set_spi_interface(zhost, spi_channel_cs1, SPI_MODE, slot);
  spiWrite(spi_channel, dac_ctrl0_reg, 2);
  spiWrite(spi_channel, dac_ctrl1_reg, 2);

  // init previous_samples to non-valid value
  for (int i = 0; i < DAC_CHANNELS_CS0; ++i) {
    poledancer->previous_samples_cs0[i] = -1;
  }
  for (int i = 0; i < DAC_CHANNELS_CS1; ++i) {
    poledancer->previous_samples_cs1[i] = -1;
  }


  // set DAC Ctrl Ref to minimum value
  // this can only happen after:
  // (1) VCA calibration call
  // (2) DAC ctrl reg init
  // spi_channel should be set to spi_channel_cs1
  spiWrite(spi_channel,
           (char*) &poledancer->dac_characterization->calibrated_codes[0][0],
           2);


  poledancer->tunable.dac_size = TWELVE_BITS;
  poledancer->tunable.dac_calibration_table = (int16_t*)calloc(TWELVE_BITS, sizeof(int16_t));
  poledancer->tunable.tune_points = (struct tune_point*)calloc(NUM_TUNING_POINTS, sizeof(struct tune_point));
  poledancer->tunable.tune_points_size = NUM_TUNING_POINTS;

  // on init use a linear tuning.  Autotune not in effect.
  create_linear_tuning(channel_map_cs0[cutoff_cv_channel],
                       poledancer->tunable.dac_size,
                       poledancer->tunable.dac_calibration_table);


  return poledancer;
}


void free_zcard(void *zcard_plugin) {
  struct poledancer_card *poledancer = (struct poledancer_card*)zcard_plugin;

  if (zcard_plugin) {
    free(poledancer->tunable.dac_calibration_table);
    free(poledancer->tunable.tune_points);

    if (poledancer->i2c_handle >= 0) {
      i2cClose(poledancer->i2c_handle);
    }
    free(poledancer);
  }
}

char* get_plugin_name() {
  return "Zoxnoxious Pole Dancer";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = DAC_CHANNELS_CS0 + DAC_CHANNELS_CS1;
  props->spi_mode = SPI_MODE;
  return props;
}



int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;
  char samples_to_dac[2];
  int spi_channel;


  spi_channel = set_spi_interface(zcard->zhost, spi_channel_cs0, SPI_MODE, zcard->slot);

  for (int i = 0; i < DAC_CHANNELS_CS0; ++i) {
    if (zcard->previous_samples_cs0[i] != samples[i] ) {
      if (samples[i] >= 0) {
        zcard->previous_samples_cs0[i] = samples[i];

        if (i == cutoff_cv_channel) {
          spiWrite(spi_channel, (char*) &zcard->tunable.dac_calibration_table[ samples[i] >> 3 ], 2);
        }
        else {
          samples_to_dac[0] = channel_map_cs0[i] | ((uint16_t) samples[i]) >> 11;
          samples_to_dac[1] = ((uint16_t) samples[i]) >> 3;
          spiWrite(spi_channel, samples_to_dac, 2);
        }
      }
      else {
        zcard->previous_samples_cs0[i] = 0;
        samples_to_dac[0] = channel_map_cs0[i] | (uint16_t) 0;
        samples_to_dac[1] = 0;
        spiWrite(spi_channel, samples_to_dac, 2);
      }

    }
  }


  // offset samples index by number of channels CS0 took
  const int16_t *samples_cs1 = &samples[DAC_CHANNELS_CS0];
  spi_channel = set_spi_interface(zcard->zhost, spi_channel_cs1, SPI_MODE, zcard->slot);

  for (int i = 0; i < DAC_CHANNELS_CS1; ++i) {
    if (zcard->previous_samples_cs1[i] != samples_cs1[i] ) {
      if (samples_cs1[i] >= 0) {
        zcard->previous_samples_cs1[i] = samples_cs1[i];
        samples_to_dac[0] = channel_map_cs1[i] | ((uint16_t) samples_cs1[i]) >> 11;
        samples_to_dac[1] = ((uint16_t) samples_cs1[i]) >> 3;
        spiWrite(spi_channel, samples_to_dac, 2);
      }
      else {
        zcard->previous_samples_cs1[i] = 0;
        samples_to_dac[0] = channel_map_cs1[i] | (uint16_t) 0;
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
  // muxes: these are wired same as the z3372, in reverse.  That is, the low number
  // cards on the upper end of the mux and high number cards on the low mux lines.
  // "fix it in software" by having the low MIDI program select the high mux address.
  // That gives the interface that MIDI prog 0 card1/out1.
  { 0, port0_addr, 0b00001111, 0b11111111 }, // prog 0 - sig1 select card1/out1
  { 0, port0_addr, 0b00001101, 0b11111101 }, // prog 1 - sig1 select card1/out2
  { 0, port0_addr, 0b00001011, 0b11111011 }, // prog 2 - sig1 select card2/out2
  { 0, port0_addr, 0b00001001, 0b11111001 }, // prog 3 - sig1 select card3/out1
  { 0, port0_addr, 0b00000111, 0b11110111 }, // prog 4 - sig1 select card4/out2
  { 0, port0_addr, 0b00000101, 0b11110101 }, // prog 5 - sig1 select card5/out1
  { 0, port0_addr, 0b00000011, 0b11110011 }, // prog 6 - sig1 select card6/out2
  { 0, port0_addr, 0b00000001, 0b11110001 }, // prog 7 - sig1 select card7/out1

  { 0, port0_addr, 0b11110000, 0b11111111 }, // prog 8 - sig2 select card1/out1
  { 0, port0_addr, 0b11010000, 0b11011111 }, // prog 9 - sig2 select card1/out2
  { 0, port0_addr, 0b10110000, 0b10111111 }, // prog 10 - sig2 select card2/out2
  { 0, port0_addr, 0b10010000, 0b10011111 }, // prog 11 - sig2 select card3/out1
  { 0, port0_addr, 0b01110000, 0b01111111 }, // prog 12 - sig2 select card4/out2
  { 0, port0_addr, 0b01010000, 0b01011111 }, // prog 13 - sig2 select card5/out1
  { 0, port0_addr, 0b00110000, 0b00111111 }, // prog 14 - sig2 select card6/out2
  { 0, port0_addr, 0b00010000, 0b00011111 }, // prog 15 - sig2 select card7/out1

  // resonance compensation selection: midi program 16 - 23
  { 1, port1_addr, 0b00000000, 0b11111101 }, // prog 16 - QVCA noninvert
  { 1, port1_addr, 0b00000010, 0b11111111 }, // prog 17 - QVCA invert
  { 1, port1_addr, 0b00000000, 0b11111011 }, // prog 18 - rez stage 1 disable
  { 1, port1_addr, 0b00000100, 0b11111111 }, // prog 19 - rez stage 1 enable
  { 1, port1_addr, 0b00000000, 0b11101111 }, // prog 20 - rez stage 3 disable
  { 1, port1_addr, 0b00010000, 0b11111111 }, // prog 21 - rez stage 3 enable
  { 1, port1_addr, 0b00000000, 0b10111111 }, // prog 22 - rez stage 2 disable
  { 1, port1_addr, 0b01000000, 0b11111111 },  // prog 23 - rez stage 2 enable

  // program shortcuts for the usable resonance compensation modes
  { 1, port1_addr, 0b00000010, 0b00000010 }, // prog 24 - no compensation
  { 1, port1_addr, 0b01010000, 0b01010000 }, // prog 25 - 4P bandpass
  { 1, port1_addr, 0b00000110, 0b00000110 }, // prog 26 - 2P alt bandpass
  { 1, port1_addr, 0b01000110, 0b01000110 }, // prog 27 - alt compensation

};


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  int error = 0;
  struct poledancer_card *zcard = (struct poledancer_card*)zcard_plugin;

  INFO("Poledancer: received program change to 0x%X", program_number);

  if (program_number < ( sizeof(midi_program_to_gpio) / sizeof(struct midi_program_to_gpio) ) ) {
    const struct midi_program_to_gpio *prog_gpio_entry = &midi_program_to_gpio[program_number];

    zcard->pca9555_port[ prog_gpio_entry->port ] |= prog_gpio_entry->set_bits;
    zcard->pca9555_port[ prog_gpio_entry->port ] &= prog_gpio_entry->clear_bits;
    error = i2cWriteByteData(zcard->i2c_handle,
                             prog_gpio_entry->gpio_reg,
                             zcard->pca9555_port[ prog_gpio_entry->port ]);

  }
  else {
    WARN("Poledancer: unexpected midi program number: 0x%X", program_number);
  }

  return error;
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



static void calibrate_vca2190_dac(struct poledancer_card *zcard, int i2c_rom_handle) {
  char rom_data[ROM_READ_SIZE_BYTES];
  int8_t board_version_number;


  if (zcard == NULL) {
    return;
  }

  // these should be read from I2C ROM.  Add channel.
  struct dac_channel_descriptor dac_channel_descriptors[NUM_CHANNEL_DESCRIPTORS] = {
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x00 }, // ctrl ref
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x07 }, // dry
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x04 }, // pole 1
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x02 }, // pole 2
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x03 }, // pole 3
    {.Vmin_measured = 0.0f, .Vmax_measured = 2.5f, .wire_prefix = 0x01 }  // pole 4
  };


  i2cReadI2CBlockData(i2c_rom_handle,
                      load_rom_address,
                      rom_data,
                      sizeof(rom_data));

  board_version_number = (int8_t)rom_data[0];

  if (board_version_number >= min_board_version) {
    for (int i = 0; i < NUM_CHANNEL_DESCRIPTORS; ++i) {
      // using memcpy: storage happened on this architecture,
      // restoring here shouldn't need to worry about endianness.
      int16_t tmp1, tmp2;
      memcpy(&tmp1, &rom_data[1 + (2 * i) * sizeof(int16_t)], sizeof(int16_t));
      dac_channel_descriptors[i].Vmin_measured = tmp1 / 1000.f;
      memcpy(&tmp2, &rom_data[1 + (2 * i + 1) * sizeof(int16_t)], sizeof(int16_t));
      dac_channel_descriptors[i].Vmax_measured = tmp2 / 1000.f;
      INFO("dac %d: %g -- %g (%hd -- %hd)",
           dac_channel_descriptors[i].wire_prefix,
           dac_channel_descriptors[i].Vmin_measured,
           dac_channel_descriptors[i].Vmax_measured,
           tmp1, tmp2);
    }
  }
  else {
    INFO("invalid board version num (%d), defaulting DAC calibration", board_version_number);
  }

  if ((zcard->dac_characterization = init_vca_dac2190_calibration(dac_channel_descriptors)) == NULL) {
    ERROR("failed to init calibration for DAC / SSI2190");
    return;
  }

  if (calculate_calibration_parameters(zcard->dac_characterization)) {
    WARN("VCA calibration measurements inconsistent-- falling back to sane values");
  }

  if (generate_channel_calibrated_codes(zcard->dac_characterization)) {
    WARN("VCA calibration failed -- uncertain results ahead");
  }
  else {
    INFO("VCA calibration complete");
  }

}
