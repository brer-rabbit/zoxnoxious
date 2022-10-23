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

#include <libconfig.h>

#ifndef ZALSA_H
#define ZALSA_H

// config lookup keys
#define ZALSA_DEVICES "zalsa.devices"

// forward reference
struct alsa_pcm_state;


/** init_alsa_device
 *
 * Initialize an alsa pcm device.  Call with a numeric that
 * indexes to a libconfig file.  Huh, that's kinda a wonky
 * interface.
 */
struct alsa_pcm_state* init_alsa_device(config_t *cfg, int device_num);


#endif
