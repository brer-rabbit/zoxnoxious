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


#ifndef CARD_H
#define CARD_H

#include "zoxnoxiousd.h"


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
struct plugin_card* discover_cards(int i2c_address, int *num_cards_found);


int load_card_plugin(struct plugin_card *plugin_card);

#endif
