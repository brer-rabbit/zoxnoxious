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
#define TEST_ALL_CARDS_TUNED(card_record) card_record

static const int card_set_sleep_time = 50000;


struct tuning_state {
  int inited;
  uint32_t gpio_mask;
  uint32_t initial_tick;
  uint32_t last_tick;
  int32_t prev_level;
  struct tuning_measurement measurements[MAX_SLOTS];
};

// callback from gpioSetGetSamplesFuncEx
static void read_samples(const gpioSample_t *samples, int num_samples, void *userdata);
static const int tune_sampling_usec_time = 500000;




/** autotune_all_cards
 * iterate over all the cards, save state, tune, pass results, iterate
 * til all cards are done.  Then restore state.
 */
int autotune_all_cards(struct card_manager *card_mgr) {
  // track which cards have completed tuning with bitmask by card_mgr->cards
  uint32_t cards_to_tune = INIT_TUNED_RECORD(card_mgr->num_cards);
  tune_status_t tune_status;
  int tuning_iterations;
  struct tuning_state tuning_state;
  uint32_t measurement_period;


  memset(&tuning_state, 0, sizeof(struct tuning_state));

  // each card will save state
  for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
    struct plugin_card *this_card = card_mgr->card_update_order[card_num];
    tune_status = (this_card->tunereq_save_state)(this_card->plugin_object);
    if (tune_status != TUNE_CONTINUE) {
      cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
      INFO("autotune: tunereq_save_state: card %d reports tuned", card_num);
    }
  }


  // loop here since each card may require multiple tuning iterations.
  // A typical card may set a calibration point, measure, then repeat
  // for a number of cycles.  Todo: should have some "ok you exceeded
  // the iterations" function to call.  And some function to call on
  // the plugin to ack the tuning failure.
  for (tuning_iterations = 0;
       tuning_iterations < MAX_TUNING_ITERATIONS && TEST_ALL_CARDS_TUNED(cards_to_tune);
       ++tuning_iterations) {

    // create gpio mask for get samples function as we go through first loop
    tuning_state.gpio_mask = 0;

    // each card should set its next set point
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      if (TEST_CARD_TUNED(cards_to_tune, card_num)) {
        struct plugin_card *this_card = card_mgr->card_update_order[card_num];
        tune_status = (this_card->tunereq_set_point)(this_card->plugin_object);

        if (tune_status != TUNE_CONTINUE) {
          cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
          INFO("tunereq_set_point: card %d reports tuned", card_num);
        }
        else {
          // record this card's gpio: accumulate add bit to gpio mask
          tuning_state.gpio_mask |= (1 << gpio_id_by_slot[ this_card->slot ]);
        }
      }
    }

    // reset for each iteration
    tuning_state.inited = 0;
    memset(tuning_state.measurements, 0, sizeof(struct tuning_measurement) * MAX_SLOTS);

    // once all cards are set, give it a rest for a short duration to let things settle
    usleep(card_set_sleep_time);

    // set the monitor and record gpio pins, sleep, then unreg the callback
    gpioSetGetSamplesFuncEx(read_samples, tuning_state.gpio_mask, &tuning_state);
    usleep(tune_sampling_usec_time);
    gpioSetGetSamplesFuncEx(NULL, 0, NULL);


    if (tuning_state.last_tick < tuning_state.initial_tick) {
      // Timer wrap -- happens every 71.6 minutes.
      // TODO: this needs more thought behind it.  May be better to
      // just pause tunereq_set_point if near the threshold.
      INFO("autotune: timer wrap during autotune (%u last < %u initial tick)",
           tuning_state.last_tick, tuning_state.initial_tick);
      continue;
    }

    measurement_period = tuning_state.last_tick - tuning_state.initial_tick;
    measurement_period = measurement_period ? measurement_period : 1; // don't divide by zero

    // report results to each card
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      if (TEST_CARD_TUNED(cards_to_tune, card_num)) {
        struct tuning_measurement *card_measurement = &tuning_state.measurements[card_num];
        card_measurement->sampling_period = measurement_period;
        card_measurement->frequency = card_measurement->samples * 1000000.f / measurement_period;
        INFO("autotune: measurement: card %d: %f Hz (%d measurements)",
             card_num,
             card_measurement->frequency, card_measurement->samples);

        struct plugin_card *this_card = card_mgr->card_update_order[card_num];
        tune_status = (this_card->tunereq_measurement)(this_card->plugin_object,
                                                       &tuning_state.measurements[card_num]);

        if (tune_status != TUNE_CONTINUE) {
          //DEBUG("autotune: measure: %d card reported as tuned", card_num);
          cards_to_tune = SET_CARD_TUNED(cards_to_tune, card_num);
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



/** read_samples
 *
 * callback function for gpioSetGetSamplesFuncEx.  Lifted most
 * entirely from the frequency count pigpio reference.  After init'ing
 * with the initial tick time and levels, check each sample for low to
 * high transitions.  Record the apropo low to high for the
 * measurements of each gpio.
 */
static void read_samples(const gpioSample_t *samples, int num_samples, void *userdata) {
  struct tuning_state *tuning_state = (struct tuning_state*)userdata;
  int sample_index;
  int high, level;

  if (!tuning_state->inited) {
    tuning_state->inited = 1;
    tuning_state->initial_tick = samples[0].tick;
    for (int i = 0; i < MAX_SLOTS; ++i) {
      tuning_state->prev_level = samples[0].level;
    }
  }

  tuning_state->last_tick = samples[num_samples - 1].tick;

  for (sample_index = 0; sample_index < num_samples; ++sample_index) {
    // xor to find any changes, and with gpio mask to get bit of interest
    level = samples[sample_index].level;
    high = ((tuning_state->prev_level ^ level) & tuning_state->gpio_mask) & level;
    tuning_state->prev_level = level;

    if (high) { // if it's a low to high
      for (int slot = 0; slot < gpio_id_by_slot_size; ++slot) {
        if (high & (1 << gpio_id_by_slot[slot])) {
          tuning_state->measurements[slot].samples++;
        }
      }
    }

  }
}
