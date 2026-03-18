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


#include <dlfcn.h>
#include <libconfig.h>
#include <pigpio.h>
#include <stdlib.h>

#include "zoxnoxiousd.h"
#include "card_manager.h"


// config keys
#define CARD_MANAGER_KEY_NAME_PREFIX "card_manager."
static const char *config_lookup_eeprom_base_i2c_address = CARD_MANAGER_KEY_NAME_PREFIX "eeprom_base_i2c_address";


// symbols to load from the dynamic library
#define INIT_ZCARD "init_zcard"
#define FREE_ZCARD "free_zcard"
#define GET_PLUGIN_NAME "get_plugin_name"
#define GET_ZCARD_PROPERTIES "get_zcard_properties"
#define PROCESS_SAMPLES "process_samples"
#define PROCESS_MIDI "process_midi"
#define PROCESS_MIDI_PROGRAM_CHANGE "process_midi_program_change"
#define TUNEREQ_SAVE_STATE "tunereq_save_state"
#define TUNEREQ_SET_POINT "tunereq_set_point"
#define TUNEREQ_MEASUREMENT "tunereq_measurement"
#define TUNEREQ_RESTORE_STATE "tunereq_restore_state"


// uninitialized memory on the EEPROM comes back with this value
static const int uninitialized_i2c_rom_read = 0xff;

/* init_card_manager
 *
 * initialize a card manager.  Takes an open libconfig to start.
 */
struct card_manager* init_card_manager(config_t *cfg) {
  struct card_manager *mgr = (struct card_manager *)calloc(1, sizeof(struct card_manager));
  if (!mgr) {
    FATAL("failed to alloc for card_manager");
    return 0;
  }

  mgr->cfg = cfg;
  return mgr;
}



void free_card_manager(struct card_manager *card_mgr) {
  // unclear how much is actually safe here- we're likely in an abort
  // state.  Even calling log functions may not be safe...

  if (card_mgr) {
    for (int card_num = 0; card_num < MAX_SLOTS; ++card_num) {
      // tell the card it's free
      if (card_mgr->cards[card_num].free_zcard) {
        card_mgr->cards[card_num].free_zcard( card_mgr->cards[card_num].plugin_object );
        INFO("called free_zcard %d slot %d", card_num, card_mgr->cards[card_num].slot);
      }

      // close dl lib plugin
      if (card_mgr->cards[card_num].dl_plugin_lib) {
        dlclose(card_mgr->cards[card_num].dl_plugin_lib);
        INFO("dlclose'd card %d", card_num);
      }
    }

    // wrap it up
    free(card_mgr);
  }
}




int discover_cards(struct card_manager *card_mgr) {
  int i2c_handle;
  int i2c_base_address;
  int i2c_read;

  /* this should probe 8 sequential I2C addresses in the range
   * i2c_address/3 to find what is present
   */
  if (config_lookup_int(card_mgr->cfg, config_lookup_eeprom_base_i2c_address, &i2c_base_address) == CONFIG_FALSE) {
    ERROR("failed to find i2c base address in config file");
    return 1;
  }

  INFO("base address: %d 0x%x", i2c_base_address, i2c_base_address);

#ifdef MOCK_DATA
  // setup 4 3340 cards, two audio outs, and return
  // not realistic, but it's test
  card_mgr->card_ids[0] = 0x02;
  card_mgr->card_ids[1] = 0x02;
  card_mgr->card_ids[2] = 0x01;
  card_mgr->card_ids[3] = 0x00;
  card_mgr->card_ids[4] = 0x00;
  card_mgr->card_ids[5] = 0x00;
  card_mgr->card_ids[6] = 0x00;
  card_mgr->card_ids[7] = 0x01;
  card_mgr->num_cards = 4;
  return 0;
#endif

  for (int slot_num = 0; slot_num < MAX_SLOTS; ++slot_num) {
    i2c_handle = i2cOpen(1, i2c_base_address + slot_num, 0);

    if (i2c_handle >= 0) {
      // this needs to be a 32 bit type, not 8 bit
      i2c_read = i2cReadByteData(i2c_handle, 0);

      if (i2c_read == PI_BAD_HANDLE || i2c_read == PI_BAD_PARAM) {
        INFO("I2C read bad handle for slot %d", slot_num);
        i2c_read = 0;
      }
      else if (i2c_read == PI_I2C_READ_FAILED) {
        INFO("no card present slot %d", slot_num);
        i2c_read = 0;
      }
      else if (i2c_read == uninitialized_i2c_rom_read) {
        INFO("no card present slot %d", slot_num);
        i2c_read = 0;
      }
      else {
        INFO("Found card in slot %d (I2C 0x%x) with id 0x%x",
             slot_num, i2c_base_address + slot_num, i2c_read);
        card_mgr->num_cards++;
      }
      card_mgr->card_ids[slot_num] = i2c_read;

    }
    else {
      INFO("failed to open handle for I2C address 0x%x",
           i2c_base_address + slot_num);
      card_mgr->card_ids[slot_num] = 0;
    }

    i2cClose(i2c_handle);
  }

  INFO("found %d cards", card_mgr->num_cards);
  return 0;
}



#define KEY_NAME_LENGTH 32

int load_card_plugins(struct card_manager *card_mgr) {
  // iterate over card_ids, store data to plugin_card[i]
  int card_num = 0;
  char key_name[KEY_NAME_LENGTH];
  const char *dynlib_basename;
  char dynlib_fullname[128];
  struct plugin_card *card; // alias to simplify


  for (int slot_num = 0; slot_num < MAX_SLOTS; ++slot_num) {

    if (card_mgr->card_ids[slot_num] != 0) { // if the Id is not zero
      card = &card_mgr->cards[card_num++]; // alias
      card->slot = slot_num;
      // card_id ends up getting stored twice
      card->card_id = card_mgr->card_ids[slot_num];
      
      snprintf(key_name, KEY_NAME_LENGTH, CARD_MANAGER_KEY_NAME_PREFIX "plugin_ids.*%d", card_mgr->card_ids[slot_num]);

      config_lookup_string(card_mgr->cfg, key_name, &dynlib_basename);

      snprintf(dynlib_fullname, 128, "%s/lib/%s.plugin.so",
               getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME) ? getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME) : DEFAULT_ZOXNOXIOUS_DIRECTORY,
               dynlib_basename);
      INFO("using %s to get: %s",
           getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME) ? getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME) : DEFAULT_ZOXNOXIOUS_DIRECTORY,
           dynlib_fullname);

      card->dl_plugin_lib = dlopen(dynlib_fullname, RTLD_NOW|RTLD_LOCAL);
      if (card->dl_plugin_lib == NULL) {
        INFO("failed to dlopen %s: %s", dynlib_fullname, dlerror());
      }

      card->init_zcard = dlsym(card->dl_plugin_lib, INIT_ZCARD);
      if (card->init_zcard == NULL) {
        ERROR("failed find symbol " INIT_ZCARD ", %s", dlerror());
        return 1;
      }

      card->free_zcard = dlsym(card->dl_plugin_lib, FREE_ZCARD);
      if (card->free_zcard == NULL) {
        ERROR("failed find symbol " FREE_ZCARD ", %s", dlerror());
        return 1;
      }

      // no need to store the handle here- just call the function and store the name
      get_plugin_name_f get_plugin_name = dlsym(card->dl_plugin_lib, GET_PLUGIN_NAME);
      if (get_plugin_name == NULL) {
        ERROR("failed find symbol " GET_PLUGIN_NAME ", %s", dlerror());
        return 1;
      }

      card->get_zcard_properties = dlsym(card->dl_plugin_lib, GET_ZCARD_PROPERTIES);
      if (card->get_zcard_properties == NULL) {
        ERROR("failed find symbol " GET_ZCARD_PROPERTIES ", %s", dlerror());
        return 1;
      }

      card->process_samples = dlsym(card->dl_plugin_lib, PROCESS_SAMPLES);
      if (card->process_samples == NULL) {
        ERROR("failed find symbol " PROCESS_SAMPLES ", %s", dlerror());
        return 1;
      }

      card->process_midi = dlsym(card->dl_plugin_lib, PROCESS_MIDI);
      if (card->process_midi == NULL) {
        ERROR("failed find symbol " PROCESS_MIDI ", %s", dlerror());
        return 1;
      }

      card->process_midi_program_change = dlsym(card->dl_plugin_lib, PROCESS_MIDI_PROGRAM_CHANGE);
      if (card->process_midi_program_change == NULL) {
        ERROR("failed find symbol " PROCESS_MIDI_PROGRAM_CHANGE ", %s", dlerror());
        return 1;
      }

      card->tunereq_save_state = dlsym(card->dl_plugin_lib, TUNEREQ_SAVE_STATE);
      if (card->tunereq_save_state == NULL) {
        ERROR("failed find symbol " TUNEREQ_SAVE_STATE ", %s", dlerror());
        return 1;
      }

      card->tunereq_set_point = dlsym(card->dl_plugin_lib, TUNEREQ_SET_POINT);
      if (card->tunereq_set_point == NULL) {
        ERROR("failed find symbol " TUNEREQ_SET_POINT ", %s", dlerror());
        return 1;
      }

      card->tunereq_measurement = dlsym(card->dl_plugin_lib, TUNEREQ_MEASUREMENT);
      if (card->tunereq_measurement == NULL) {
        ERROR("failed find symbol " TUNEREQ_MEASUREMENT ", %s", dlerror());
        return 1;
      }

      card->tunereq_restore_state = dlsym(card->dl_plugin_lib, TUNEREQ_RESTORE_STATE);
      if (card->tunereq_restore_state == NULL) {
        ERROR("failed find symbol " TUNEREQ_RESTORE_STATE ", %s", dlerror());
        return 1;
      }

      card->plugin_name = (*get_plugin_name)();
      INFO("loaded plugin for %s", card->plugin_name);
    }

  }


  return 0;
}



struct card_spi_sort_criteria {
  int slot;
  int spi_mode;
  int num_channels;
  struct plugin_card *card;
};

static int zcard_compare_spi_f(const void *name1, const void *name2) {
  // sort criteria:
  // (1) spi mode
  // (2) num channels (max to min)
  // (3) slot number (tie breaker)
  const struct card_spi_sort_criteria *card1 = (const struct card_spi_sort_criteria*) name1;
  const struct card_spi_sort_criteria *card2 = (const struct card_spi_sort_criteria*) name2;

  if (card1->spi_mode != card2->spi_mode) {
    return card1->spi_mode > card2->spi_mode ? -1 : 1;
  }
  else if (card1->num_channels != card2->num_channels) {
    return card1->num_channels < card2->num_channels ? -1 : 1;
  }
  else {
    return card1->slot < card2->slot ? -1 : 1;
  }
  
}


void assign_update_order(struct card_manager *card_mgr) {
  struct zcard_properties *zcard_props;
  struct card_spi_sort_criteria *cards;
  cards = (struct card_spi_sort_criteria*)calloc(card_mgr->num_cards, sizeof(struct card_spi_sort_criteria));

  // initialize order by just copying
  for (int i = 0; i < card_mgr->num_cards; ++i) {
    zcard_props = card_mgr->cards[i].get_zcard_properties();
    cards[i].slot = card_mgr->cards[i].slot;
    cards[i].spi_mode = zcard_props->spi_mode;
    cards[i].num_channels = zcard_props->num_channels;
    card_mgr->cards[i].num_channels = zcard_props->num_channels; // refactor to just use this
    cards[i].card = &card_mgr->cards[i];
    free(zcard_props);
  }
  
  qsort(card_mgr->card_update_order, card_mgr->num_cards, sizeof(struct card_spi_sort_criteria*), zcard_compare_spi_f);

  INFO("sorted:");

  for (int i = 0; i < card_mgr->num_cards; ++i) {
    card_mgr->card_update_order[i] = cards[i].card;
    INFO("assign_update_order: %d : slot %d", i, cards[i].slot);
  }

  free(cards);
}



/* assign_hw_audio_channels
 *
 * Take a pointer to an array of ints, representing the number of channels each hw device has available.
 * The num_devices is the size of the array.
 */

void assign_hw_audio_channels(struct card_manager *card_mgr, int *channels, int num_devices) {
  int *bin_current_index = (int*)calloc(num_devices, sizeof(int));
  int card_idx, bin_num;

  for (card_idx = 0; card_idx < card_mgr->num_cards; ++card_idx) {
    for (bin_num = 0; bin_num < num_devices; ++bin_num) {
      if (bin_current_index[bin_num] + card_mgr->card_update_order[card_idx]->num_channels <= channels[bin_num]) {
        // store current pcm device (bin_num) as where control voltages come from.
        // store the offset to the channels there too.
        card_mgr->card_update_order[card_idx]->pcm_device_num = bin_num;
        card_mgr->card_update_order[card_idx]->channel_offset = bin_current_index[bin_num];
        bin_current_index[bin_num] += card_mgr->card_update_order[card_idx]->num_channels;
        INFO("allocated %d channels on dev %d for %s at start %d (new idx %d)",
             card_mgr->card_update_order[card_idx]->num_channels,
             bin_num, card_mgr->card_update_order[card_idx]->plugin_name,
             card_mgr->card_update_order[card_idx]->channel_offset,
             bin_current_index[bin_num]);
        break;
      }
    }
    if (bin_num == num_devices) {
      INFO("unable to find an allocation for %d %s", card_idx, card_mgr->card_update_order[card_idx]->plugin_name);
    }
  }


  free(bin_current_index);
}
