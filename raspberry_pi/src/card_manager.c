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
#include "card_manager.h"

/* Config keys */
char *config_lookup_eeprom_base_i2c_address = "card_manager.eeprom_base_i2c_address";


/* properties of a plugin_card */
struct plugin_card {
  int slot;
  int card_id;
  char *plugin_name;
  // plugin interface function pointers:
  process_samples process_samples_f;
  process_midi process_midi_f;
  process_midi_program_change process_midi_program_change_f;
  free_zcard free_zcard_f;
  void *plugin_object;
};


/* card manager which holds all the fun stuff */
struct card_manager {
  // libconfig handle
  config_t *cfg;

  // 8-bit Id of cards, indexed by physical slot, or zero if no card present:
  uint8_t card_ids[MAX_SLOTS];

  // plugins- num_cards are used; index does not represent physical
  // ordering of slots:
  struct plugin_card cards[MAX_SLOTS];
  int num_cards;
};





/* init_card_manager
 *
 * initialize a card manager.  Takes an open libconfig to start.
 */
struct card_manager* init_card_manager(config_t *cfg) {
  struct card_manager *mgr = (struct card_manager *)calloc(sizeof(struct card_manager), 1);
  if (!mgr) {
    FATAL("failed to alloc for card_manager");
    return 0;
  }

  mgr->cfg = cfg;
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


int discover_cards(struct card_manager *card_mgr) {
  int i2c_handle;
  int i2c_base_address;

  /* this should probe 8 sequential I2C addresses in the range
   * i2c_address/3 to find what is present
   */
  config_lookup_int(card_mgr->cfg, config_lookup_eeprom_base_i2c_address, &i2c_base_address);

  for (int slot_num = 0; slot_num < MAX_SLOTS; ++slot_num) {
    i2c_handle = i2cOpen(1, i2c_base_address + slot_num, 0);
    if (i2c_handle >= 0) {
      card_mgr->card_ids[slot_num] = i2cReadByteData(i2c_handle, 0);

      if (card_mgr->card_ids[slot_num] == PI_BAD_HANDLE ||
          card_mgr->card_ids[slot_num] == PI_BAD_PARAM) {
        INFO("I2C read bad handle for slot %d", slot_num);
        card_mgr->card_ids[slot_num] = 0;
      }
      else if (card_mgr->card_ids[slot_num] == PI_I2C_READ_FAILED) {
        INFO("no card present slot %d", slot_num);
        card_mgr->card_ids[slot_num] = 0;
      }
      else {
        INFO("Found card in slot %d with id 0x%x",
             slot_num, card_mgr->card_ids[slot_num]);
        card_mgr->num_cards++;
      }

      i2cClose(i2c_handle);
    }
    else {
      INFO("failed to open handle for address %d",
           i2c_base_address + slot_num);
      card_mgr->card_ids[slot_num] = 0;
    }

  }

  INFO("found %d cards", card_mgr->num_cards);
  return 0;
}



int load_card_plugins(struct card_manager *card_mgr) {
  // iterate over card_ids, store data to plugin_card[i]
  int card_num = 0;


  for (int slot_num = 0; slot_num < MAX_SLOTS; ++slot_num) {
    if (card_mgr->card_ids[slot_num] != 0) {
      card_mgr->cards[card_num].slot = slot_num;
      // card_id ends up getting stored twice
      card_mgr->cards[card_num].card_id = card_mgr->card_ids[slot_num];
      
    }
  }


  return 0;
}
