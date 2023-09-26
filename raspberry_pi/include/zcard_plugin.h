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

#ifndef ZCARD_PLUGIN_H
#define ZCARD_PLUGIN_H

#include <alsa/asoundlib.h>
#include <pigpio.h>
#include <zlog.h>
#include "zoxnoxiousd.h"

#define I2C_BUS 1


/* logging */
#define DEBUG(...) zlog_debug(zlog_c, __VA_ARGS__)
#define INFO(...)  zlog_info(zlog_c, __VA_ARGS__)
#define WARN(...)  zlog_warn(zlog_c, __VA_ARGS__)
#define ERROR(...) zlog_error(zlog_c, __VA_ARGS__)
#define FATAL(...) zlog_fatal(zlog_c, __VA_ARGS__)
extern zlog_category_t *zlog_c;



/* this is intended to be the interface for cards.  Library functions
 * available for card plugins are here as well.
 * typedef functions need to be implemented by the card driver (you)..
 */


/* passed in to the init_zcard_f, this object is needed to pass to
 * some helper functions.  So store it during init, put it away, bring
 * it out during process_samples_f or whatevz.
 */
struct zhost;
struct zhost* zhost_create();

/* set_spi_interface
 * must be called by plugin prior any function's spiWrite() or when changing spi mode changes
 * in a function.  Wraps/caches pigpio spiOpen and provides a return consistent with spiOpen.
 * A valid handle can be used for pigpio's spiWrite.  Do not close the handle.
 */
int set_spi_interface(struct zhost *zhost, unsigned int spi_channel, unsigned int spi_mode, int slot);


/** init the plugin.
 * Input: slot number for the card (0-7).
 * Any setup/state should be done here (constructor).  Return a
 * pointer to an object that will be used for subsequent calls
 * (zcard_plugin in other calls).
 * Any i2cOpen calls should be done here with the handle cached.
 */
typedef void* (*init_zcard_f)(struct zhost *zhost, int slot);

/* free_zcard
 *
 * called prior to card deletion / plugin removal.
 */
typedef void (*free_zcard_f)(void *zcard_plugin);


/** get_plugin_name
 *
 * return the plugin name in under 32 chars with a null terminator.
 * Caller is responsible for free'ing memory.
 */
typedef char* (*get_plugin_name_f)();



struct zcard_properties {
  int num_channels;  // number of channles the card/plugin requires.
  int spi_mode;  // spi mode used. if >1 mode, plugin should set most latency sensitive mode.
};

/** get_zcard_properties
 *
 * the plugin retuns a pointer to struct of properties.  The caller
 * will be responsible for release of memory.
 * num_channels is used for allocating where the card goes in channel mapping.
 * spi_mode is used to optimize calling plugins that have the same spi_mode in sequence.
 * 0,1,2,3 are valid spi modes.
 */
typedef struct zcard_properties* (*get_zcard_properties_f)();




/** process_samples
 *
 * Plugin interface to receives samples and a spi handle.
 * Take a handle to the SPI channel and an array of samples to send to
 * card.  Card is responsible for sending the samples down the SPI
 * channel.  This method should call set_spi_interface() prior to sending
 * any samples or changing spi mode.
 */
typedef int (*process_samples_f)(void *zcard_plugin, const int16_t *samples);



/** process_midi
 *
 * process a midi message for the card.  Message is filtered to be for this plugin.
 * This interface is for non-program change messages.
 */
typedef int (*process_midi_f)(void *zcard_plugin, uint8_t *midi_message, size_t size);


/** process_midi_program_change
 *
 * process a midi message program change for this plugin.
 */
typedef int (*process_midi_program_change_f)(void *zcard_plugin, uint8_t program_number);


/** Tuning

 * tuning is done in parallel across all cards.
 * 1. The zoxnoxiousd caller notes all cards as "untuned".
 * Each card:
 * 2. Save state with tunereq_save_state_f
 * 3. Set next calibration/measurement point with tunereq_set_next_f
 * 4. Passed results of measurement with tunereq_results_f
 * 5. Restore state with tunereq_restore_state_f
 *
 * Steps 3 and 4 are repeated until all cards report tuning complete.
 * A maximum of 32 iterations is allowed.
 */

/** tunereq_save_state_f
 *
 * Start of a tune request.  The tune request may be for this card or
 * it may be for a different card.  Regardless the card is expected to:
 * (1) save any state that may be manipulated during tuning
 * (2) mute any outputs if applicable
 * Set any state to avoid any MIDI updates so they can be queued.  Anything
 * coming in via USB Audio PCM shouldn't need to be saved.
 *
 * Return zero on success, non-zero on failure.
 */
typedef int (*tunereq_save_state_f)(void *zcard_plugin);

/** tunereq_tune_card
 *
 * Tune this card.  The method has complete control over setting any
 * values necessary to complete tuning.  All other cards should be
 * mute and prepared for tuning.
 * The card can consider storing tuning results to restore on the next boot.
 *
 * Return zero on success, non-zero on failure.
 */
typedef int (*tunereq_tune_card_f)(void *zcard_plugin);

/** tunereq_restore_state_f
 *
 * A tune request completed.  This may be for this card or it may be
 * for a different card.  Either way, the card is expected to restore
 * state to where it was at the beginning of the tune request when
 * tunereq_save_state_f was called.
 *
 * Return zero on success, non-zero on failure.
 */
typedef int (*tunereq_restore_state_f)(void *zcard_plugin);


/** convenience for mapping: find the gpio that a slot drives
 */
extern const int gpio_id_by_slot[];


#endif
