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


#ifndef CARD_MANAGER_H
#define CARD_MANAGER_H

#include "zoxnoxiousd.h"


struct card_manager;

/* properties of a plugin_card */
struct plugin_card {
  int slot;
  int card_id;
  char *plugin_name;
  int i2c_handle;
  // plugin interface function pointers:
  process_samples process_samples_f;
  process_midi process_midi_f;
  process_midi_program_change process_midi_program_change_f;
  free_zcard free_zcard_f;
  void *plugin_object;
};



/* init_card_manager
 *
 * start the card manager.  Give it a handle to the libconfig.
 */
struct card_manager* init_card_manager(config_t *cfg);


/* discover_cards
 *
 * Search for all M24C02 ROMs.
 * given a base i2c address, probe all addresses for the last 3 bits
 * (8 addresses total).  If a response is received, read the first
 * byte from the ROM for the card id.
 * Returns:
 * Array of plugin_cards found
 * number of cards found set at int*
 */
int discover_cards(struct card_manager *card_mgr);


int load_card_plugin(struct plugin_card *plugin_card);

#endif
