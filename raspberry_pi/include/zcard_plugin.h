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


/* this is intended to be the interface for cards.  Library functions
 * available for card plugins are here as well.
 * typedef functions need to be implemented.
 */


/* set_spi_interface
 * must be called by plugin prior to initial spiWrite() or where the spi mode changes
 */
int set_spi_interface(int spi_mode, int slot);




/** init the plugin.
 * Input: slot number for the card (0-7).
 * Any setup/state should be done here (constructor).  Return a
 * pointer to an object that will be used for subsequent calls
 * (zcard_plugin in other calls).
 * Any i2cOpen calls should be done here with the handle cached.
 */
typedef void* (*init_zcard_f)(int slot);

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
typedef int (*process_samples_f)(void *zcard_plugin, int16_t *samples);



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



#endif
