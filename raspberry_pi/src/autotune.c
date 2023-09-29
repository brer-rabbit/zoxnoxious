/* Copyright 2023 Kyle Farrell
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

/* auto tune, cpu tune, whatevz.  Functions for orchestrating the
 * tuning on the individual cards, measuring gpio frequency, and
 * seeing tune to completion.
 */

#include "autotune.h"
#include "zcard_plugin.h"


#define MAX_TUNING_ITERATIONS 32
#define INIT_TUNED_RECORD(num_cards)  ((1 << num_cards) - 1)
#define SET_CARD_TUNED(card_record, this_card) card_record & ~(1 << this_card)
#define TEST_CARD_TUNED(card_record, this_card) card_record & (1 << this_card)


struct tuning_state {
  int inited;
  uint32_t initial_low_to_high_tick;
  uint32_t last_low_to_high_tick;
  int32_t prev_level[MAX_SLOTS];
  struct tuning_measurement measurements[MAX_SLOTS];
};

// callback from gpioSetGetSamplesFuncEx
static void read_samples(const gpioSample_t *samples, int num_samples, void *userdata);
static const int tune_sampling_usec_time = 100000;




/** autotune_all_cards
 * iterate over all the cards, save state, tune, pass results, iterate
 * til all cards are done.  Then restore state.
 */
int autotune_all_cards(struct card_manager *card_mgr) {
  // track which cards have completed tuning with bitmask by card_mgr->cards
  uint32_t cards_to_tune = INIT_TUNED_RECORD(card_mgr->num_cards);
  tune_status_t tune_status;
  int tuning_iterations;
  int gpio_mask;
  struct tuning_state tuning_state;

  // each card will save state
  for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
    struct plugin_card *this_card = card_mgr->card_update_order[card_num];
    tune_status = (this_card->tunereq_save_state)(this_card->plugin_object);
    if (tune_status != TUNE_CONTINUE) {
      cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
      INFO("autotune: save state: card %d tuned (cards: %d)", card_num, cards_to_tune);
    }
  }


  // loop here since each card may require multiple tuning iterations.  Typical would
  // be to set a calibration point, measure, then repeat for a number of cycles.
  // Todo: should have some "ok you exceeded the iterations" function to call.  And
  // some function to call on the plugin to ack the tuning failure.
  for (tuning_iterations = 0; tuning_iterations < MAX_TUNING_ITERATIONS; ++tuning_iterations) {

    gpio_mask = 0; // create gpio mask as we go through first loop

    // each card should set its next set point
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      if (TEST_CARD_TUNED(cards_to_tune, card_num)) {
        struct plugin_card *this_card = card_mgr->card_update_order[card_num];
        tune_status = (this_card->tunereq_set_point)(this_card->plugin_object);

        gpio_mask |= (1 << gpio_id_by_slot[ this_card->slot ]);

        if (tune_status != TUNE_CONTINUE) {
          cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
          INFO("autotune: set point: %d card tuned (cards: %d)", card_num, cards_to_tune);
        }
      }
    }

    // set the monitor and record gpio pins
    gpioSetGetSamplesFuncEx(read_samples, gpio_mask, &tuning_state);

    usleep(tune_sampling_usec_time);
    /*
    while (tuning_state.low_to_high_count < MAX_TUNING_SAMPLES &&
           ++attempt_num < tuning_sampling_retries) {
      usleep(tune_sampling_usec_time);
    }
    */

    // unregister callback
    gpioSetGetSamplesFuncEx(NULL, 0, NULL);


    // report results to each card
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      if (TEST_CARD_TUNED(cards_to_tune, card_num)) {
        struct plugin_card *this_card = card_mgr->card_update_order[card_num];
        tune_status = (this_card->tunereq_measurement)(this_card->plugin_object, NULL);
        if (tune_status != TUNE_CONTINUE) {
          cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
          INFO("autotune: measure: %d card tuned (cards: %d)", card_num, cards_to_tune);
        }
      }
    }

  }

  // restore state
  for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
    (card_mgr->card_update_order[card_num]->tunereq_restore_state)(card_mgr->card_update_order[card_num]->plugin_object);
  }

  return 0;
}




static void read_samples(const gpioSample_t *samples, int num_samples, void *userdata) {
  struct tuning_state *tuning_state = (struct tuning_state*)userdata;
  int sample_index;
  int high, level;

  if (!tuning_state->inited) {
    tuning_state->inited = 1;
    for (int i = 0; i < MAX_SLOTS; ++i) {
      tuning_state->prev_level[i] = samples[0].level;
    }
  }

  for (sample_index = 0; sample_index < num_samples; ++sample_index) {
    // xor to find any changes, and with gpio mask to get bit of interest
    level = samples[sample_index].level;
    high = ((tuning_state->prev_level ^ level) & tuning_state->gpio_mask) & level;
    tuning_state->prev_level = level;

    if (high) { // if it's a low to high
      if (tuning_state->low_to_high_count++ == 0) { // count will be 1 greater than number of cycles
        tuning_state->initial_low_to_high_tick = samples[sample_index].tick;
      }

      tuning_state->last_low_to_high_tick = samples[sample_index].tick;
    }
  }

}
