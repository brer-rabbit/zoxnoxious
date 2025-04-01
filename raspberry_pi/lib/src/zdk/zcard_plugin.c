/* Copyright 2022 Kyle Farrell
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

#include "zcard_plugin.h"

//#define SPI_RATE 12000000
#define SPI_RATE 30000000

#define INITIAL_SPI_FLAGS 1
#define INITIAL_SLOT 5

// GPIOs 13, 19, 26 mux the chip select to the different cards
#define MUXOUT_0 13
#define MUXOUT_1 19
#define MUXOUT_2 26


#define NUM_SPI_CHIP_SELECTS 2

/* zlog loggin' */
zlog_category_t *zlog_c = NULL;

struct spi_device {
    int spi_flags;
    int spi_handle;
};

struct zhost {
    struct spi_device spi_devices[NUM_SPI_CHIP_SELECTS];
    int active_slot;
//    int spi_flags;
//    int spi_handle;
};


struct gpio_pin_value_tuple {
    unsigned int gpio;
    unsigned int level;
};

// mind bit ordering here vs usage...MSB is a thing
static const struct gpio_pin_value_tuple pin_value_tuple[8][3] = {
  { { MUXOUT_2, 0 }, { MUXOUT_1, 0 }, { MUXOUT_0, 0 } },
  { { MUXOUT_2, 0 }, { MUXOUT_1, 0 }, { MUXOUT_0, 1 } },
  { { MUXOUT_2, 0 }, { MUXOUT_1, 1 }, { MUXOUT_0, 0 } },
  { { MUXOUT_2, 0 }, { MUXOUT_1, 1 }, { MUXOUT_0, 1 } },
  { { MUXOUT_2, 1 }, { MUXOUT_1, 0 }, { MUXOUT_0, 0 } },
  { { MUXOUT_2, 1 }, { MUXOUT_1, 0 }, { MUXOUT_0, 1 } },
  { { MUXOUT_2, 1 }, { MUXOUT_1, 1 }, { MUXOUT_0, 0 } },
  { { MUXOUT_2, 1 }, { MUXOUT_1, 1 }, { MUXOUT_0, 1 } }
};


struct zhost* zhost_create() {
  struct zhost *zhost = (struct zhost*)malloc(sizeof(struct zhost));

  if (zhost == NULL) {
    return NULL;
  }

  if (gpioSetMode(MUXOUT_0, PI_OUTPUT) ||
      gpioSetMode(MUXOUT_1, PI_OUTPUT) ||
      gpioSetMode(MUXOUT_2, PI_OUTPUT)) {
    ERROR("failed to open SPI");
    free(zhost);
    return NULL;
  }

  if (gpioSetMode(pin_value_tuple[INITIAL_SLOT][0].gpio, PI_OUTPUT) ||
      gpioSetMode(pin_value_tuple[INITIAL_SLOT][1].gpio, PI_OUTPUT) ||
      gpioSetMode(pin_value_tuple[INITIAL_SLOT][2].gpio, PI_OUTPUT)) {
    ERROR("gpio setMode failed");
    free(zhost);
    return NULL;
  }

  if (gpioWrite(pin_value_tuple[INITIAL_SLOT][0].gpio, pin_value_tuple[INITIAL_SLOT][0].level) ||
      gpioWrite(pin_value_tuple[INITIAL_SLOT][1].gpio, pin_value_tuple[INITIAL_SLOT][1].level) ||
      gpioWrite(pin_value_tuple[INITIAL_SLOT][2].gpio, pin_value_tuple[INITIAL_SLOT][2].level)) {
    ERROR("gpio write failed");
    free(zhost);
    return NULL;
  }

  zhost->active_slot = INITIAL_SLOT;
  for (int i = 0; i < NUM_SPI_CHIP_SELECTS; ++i) {
      zhost->spi_devices[i].spi_flags = INITIAL_SPI_FLAGS;
      if ((zhost->spi_devices[i].spi_handle = spiOpen(i, SPI_RATE, zhost->spi_devices[i].spi_flags)) < 0) {
          ERROR("failed to open SPI channel %d", i);
          free(zhost);
          return NULL;
      }
  }


  return zhost;
}


int set_spi_interface(struct zhost *zhost, unsigned int spi_channel, unsigned int spi_flags, int slot) {

    assert(spi_channel < NUM_SPI_CHIP_SELECTS);

    if (zhost->active_slot != slot) {
        // change GPIOs to the slot
        if (gpioWrite(pin_value_tuple[slot][0].gpio, pin_value_tuple[slot][0].level) ||
            gpioWrite(pin_value_tuple[slot][1].gpio, pin_value_tuple[slot][1].level) ||
            gpioWrite(pin_value_tuple[slot][2].gpio, pin_value_tuple[slot][2].level)) {
            ERROR("gpio write failed");
            return -1;  // valid spi handle is >= 0
        }
        zhost->active_slot = slot;
    }


    // get a SPI handle- return the existing one if it's a match
    if (zhost->spi_devices[spi_channel].spi_flags != spi_flags) {
        if ( spiClose(zhost->spi_devices[spi_channel].spi_handle) != 0 ) {
            ERROR("spiClose failed");
        }
        if ( (zhost->spi_devices[spi_channel].spi_handle =
              spiOpen(spi_channel, SPI_RATE, spi_flags)) < 0) {
            ERROR("spiOpen failed");
        }
        zhost->spi_devices[spi_channel].spi_flags = spi_flags;
    }

    return zhost->spi_devices[spi_channel].spi_handle;
}


const int gpio_id_by_slot[] = { 17, 27, 22, 23, 24, 25 };
const int gpio_id_by_slot_size = sizeof(gpio_id_by_slot) / sizeof(int);
