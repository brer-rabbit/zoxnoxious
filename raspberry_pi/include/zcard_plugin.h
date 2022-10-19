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
 * must be called by plugin prior to the initial spiWrite()
 */
int set_spi_interface(int slot, int spi_mode);


/* init the plugin.
 * Input: slot number for the card (0-7).
 * Any setup/state should be done here (constructor).  Return a
 * pointer to an object that will be used for subsequent calls
 * (zcard_plugin in other calls).
 * Any i2cOpen calls should be done here with the handle cached.
 */
typedef void* (*init_zcard)(int slot);

/* free_zcard
 *
 * called prior to card deletion / plugin removal.
 */
typedef void (*free_zcard)(void *zcard_plugin);


/* get_plugin_name
 *
 * return the plugin name in under 32 chars with a null terminator.
 * Caller is responsible for free'ing memory.
 */
typedef char* (*get_plugin_name)();


/** process_samples
 *
 * Plugin interface to receives samples and a spi handle.
 * Take a handle to the SPI channel and an array of samples to send to
 * card.  Card is responsible for sending the samples down the SPI
 * channel.
 * Pre: SPI handle will be set to the correct mode that the plugin requires.
 */
typedef int (*process_samples)(void *zcard_plugin, int16_t *samples, int spi_handle);



/** process_midi
 *
 * process a midi message for the card.  Message is filtered to be for this plugin.
 * This interface is for non-program change messages.
 */
typedef int (*process_midi)(void *zcard_plugin, uint8_t *midi_message, size_t size);


/** process_midi_program_change
 *
 * process a midi message program change for this plugin.
 */
typedef int (*process_midi_program_change)(void *zcard_plugin, uint8_t program_number);



#endif
