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


#define MAX_SLOTS 8


struct card_manager;

struct plugin_card;


/** init_card_manager
 *
 * start the card manager.  Give it a handle to the libconfig.
 */
struct card_manager* init_card_manager(config_t *cfg);


/** discover_cards
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


/** load_card_plugins
 * load the dynamic lib plugins for each card.  Store function
 * pointers for the functions needed, get the card name from the
 * plugin and store that too.
 */
int load_card_plugins(struct card_manager *card_mgr);


/** assign_update_order
 *
 * assign the order for updates of the card to minimize changes in spi mode.
 * pre: plugins loaded via discover cards
 * post: an ordering for the plugin cards is determined and available
 */
void assign_update_order(struct card_manager *card_mgr);


#endif
