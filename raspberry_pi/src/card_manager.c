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

/* responsible for discovery of cards in the system, loading plugin
 * modules for them and all that
 */


#include <libconfig.h>
#include <pigpio.h>
#include <stdlib.h>
#include <zlog.h>

#include "zoxnoxiousd.h"


struct card_manager {
  uint8_t card_ids[8];  // 8-bit Id of each card indexed by slot, or zero if no card present

};


struct card_manager* init_card_manager(config_t *cfg) {
  struct card_manager *mgr = (struct card_manager *)malloc(sizeof(struct card_manager));
  if (!mgr) {
    FATAL("failed to alloc for card_manager");
    return 0;
  }
  return mgr;
}



  /*
  *num_cards_found = 3;
  *plugin_cards = (struct plugin_card*) calloc(3, sizeof(struct plugin_card));

  *plugin_cards[0] = (struct plugin_card)
    {
      .slot = 0,
      .card_id = 0x02,
      .plugin_name = "VCO3340",
      .process_samples_f = NULL,
      .process_midi_f = NULL,
      .process_midi_program_change_f = NULL,
      .free_zcard_f = NULL
    };
  */


struct plugin_card* discover_cards(int i2c_address, int *num_cards_found) {
  int i2c_handle;
  uint8_t card_ids[8];

  /* this should probe 8 sequential I2C addresses in the range
   * i2c_address to find what is present
   */

  *num_cards_found = 0;

  for (int i = 0; i < 8; ++i) {
    i2c_handle = i2cOpen(1, i2c_address + i, 0);
    if (i2c_handle >= 0) {
      card_ids[i] = i2cReadByteData(i2c_handle, 0);

      if (card_ids[i] == PI_BAD_HANDLE ||
          card_ids[i] == PI_BAD_PARAM) {
        INFO("I2C read bad handle for slot %d", i);
        card_ids[i] = 0;
      }
      else if (card_ids[i] == PI_I2C_READ_FAILED) {
        INFO("no card present slot %d", i);
        card_ids[i] = 0;
      }
      else {
        INFO("Found card in slot %d with id 0x%x", i, card_ids[i]);
        *num_cards_found++;
      }

    }
    else {
      INFO("failed to open handle for address %d\n", i2c_address + i);
      card_ids[i] = 0;
    }
  }


  return 0;
}




