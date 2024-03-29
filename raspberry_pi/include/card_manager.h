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

#include <libconfig.h>

#include "zoxnoxiousd.h"

#define MAX_SLOTS 8


/* properties of a plugin_card */
struct plugin_card {
  int slot;
  int card_id;
  char *plugin_name;

  // find this card's channels in the stream via offset
  int pcm_device_num;
  int channel_offset;
  int num_channels;
  
  // plugin interface function pointers:
  void *dl_plugin_lib;
  init_zcard_f init_zcard;
  get_zcard_properties_f get_zcard_properties;
  process_samples_f process_samples;
  process_midi_f process_midi;
  process_midi_program_change_f process_midi_program_change;
  tunereq_save_state_f tunereq_save_state;
  tunereq_set_point_f tunereq_set_point;
  tunereq_measurement_f tunereq_measurement;
  tunereq_restore_state_f tunereq_restore_state;
  free_zcard_f free_zcard;
  void *plugin_object;
};


/* card manager which holds all the fun stuff */
struct card_manager {
  // libconfig handle
  config_t *cfg;

  // 8-bit Id of cards, indexed by physical slot, or zero if no card present:
  uint8_t card_ids[MAX_SLOTS];

  // plugins- num_cards are used; index does not represent physical
  // ordering of slots.  This is populated in load_card_plugins().
  // (elsewhere this ends up getting used as MIDI channel)
  struct plugin_card cards[MAX_SLOTS];
  int num_cards;

  // update order for cards: order the cards to minimize latency- group cards by rules
  // such as their spi mode
  struct plugin_card *card_update_order[MAX_SLOTS];
};



/** init_card_manager
 *
 * start the card manager.  Give it a handle to the libconfig.
 */
struct card_manager* init_card_manager(config_t *cfg);



/** free_card_manager
 *
 * release all cards by calling the cards free_zcard_f method.
 * close all dynlibs.
 * free card manager.
 */
void free_card_manager(struct card_manager *card_mgr);



/** discover_cards
 *
 * Search for all M24C02 ROMs.
 * given a base i2c address, probe all addresses for the last 3 bits
 * (8 addresses total).  If a response is received, read the first
 * byte from the ROM for the card id.
 * Returns:
 * Zero for success; non-zero otherwise.
 * Populates internal data structure:
 *   foreach card
 *     --> card_ids[slot] = Id byte from ROM if found otherwise zero
 *     --> number of cards in system
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


/** assign_hw_audio_channels
 *
 * assign the channels for the cards: given the one or two USB Audio
 * streams hold 24 or 32 channels each (depending on how we config
 * this), and each card requires a number of channels, assign this
 * out.  This is basically a "bin packing problem" type of thing.
 * Since we know all the cards upfront, use something like first-fit-
 * decreasing to do the assignment:
 *  --> sort based on number of channels, max to min
 *  --> fit into first bucket (audio stream 1) or second if it doesn't fit
 * if we can't fit a card...well crud, that's an issue.  A card is not
 * split between two audio streams.
 *
 * Take a pointer to an array of ints, representing the number of channels each hw device has available.
 * The num_devices is the size of the array.
 */
void assign_hw_audio_channels(struct card_manager *card_mgr, int *channels, int num_devices);



#endif
