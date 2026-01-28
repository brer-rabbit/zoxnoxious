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
// my scope shows 24000000 to be 28MHz
#define SPI_RATE 24000000

#define INITIAL_SPI_FLAGS 1
#define INITIAL_SLOT 5

// GPIOs to mux the SPI chip selects to the different cards
#define MUXOUT_0 20
#define MUXOUT_1 26
#define MUXOUT_2 21
#define MUX_MASK ((1u << MUXOUT_0) | (1u << MUXOUT_1) | (1u << MUXOUT_2))

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


struct gpio_bitmask {
    uint32_t set_mask;
    uint32_t clear_mask;
};

#define SET_MASK_FOR_SLOT(slot) \
    ( ((slot) & 0x1 ? (1u << MUXOUT_0) : 0) | \
      ((slot) & 0x2 ? (1u << MUXOUT_1) : 0) | \
      ((slot) & 0x4 ? (1u << MUXOUT_2) : 0) )

#define CLEAR_MASK_FOR_SLOT(slot) \
    ( MUX_MASK & ~SET_MASK_FOR_SLOT(slot) )

static const struct gpio_bitmask mux_masks[8] = {
    { SET_MASK_FOR_SLOT(0), CLEAR_MASK_FOR_SLOT(0) },
    { SET_MASK_FOR_SLOT(1), CLEAR_MASK_FOR_SLOT(1) },
    { SET_MASK_FOR_SLOT(2), CLEAR_MASK_FOR_SLOT(2) },
    { SET_MASK_FOR_SLOT(3), CLEAR_MASK_FOR_SLOT(3) },
    { SET_MASK_FOR_SLOT(4), CLEAR_MASK_FOR_SLOT(4) },
    { SET_MASK_FOR_SLOT(5), CLEAR_MASK_FOR_SLOT(5) },
    { SET_MASK_FOR_SLOT(6), CLEAR_MASK_FOR_SLOT(6) },
    { SET_MASK_FOR_SLOT(7), CLEAR_MASK_FOR_SLOT(7) },
};




struct zhost* zhost_create() {
  struct zhost *zhost = (struct zhost*)malloc(sizeof(struct zhost));

  if (zhost == NULL) {
    return NULL;
  }

  if (gpioSetMode(MUXOUT_0, PI_OUTPUT) ||
      gpioSetMode(MUXOUT_1, PI_OUTPUT) ||
      gpioSetMode(MUXOUT_2, PI_OUTPUT)) {
    ERROR("gpio setMode failed");
    free(zhost);
    return NULL;
  }

  if (gpioWrite_Bits_0_31_Clear(mux_masks[INITIAL_SLOT].clear_mask) ||
      gpioWrite_Bits_0_31_Set(mux_masks[INITIAL_SLOT].set_mask)) {
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
    int retval;
    assert(spi_channel < NUM_SPI_CHIP_SELECTS);

    if (zhost->active_slot != slot) {
        // change GPIOs to the slot
        retval = gpioWrite_Bits_0_31_Clear(mux_masks[slot].clear_mask);
        retval += gpioWrite_Bits_0_31_Set(mux_masks[slot].set_mask);

        if (retval) {
            ERROR("gpio write failed");
            return -1;
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
