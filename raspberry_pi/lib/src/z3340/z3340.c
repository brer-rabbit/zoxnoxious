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

#include "zcard_plugin.h"


void* init_zcard(int slot) {
  return NULL;
}


char* get_plugin_name() {
  return "Zoxnoxious 3340";
}

int process_samples(void *zcard_plugin, int16_t *samples, int spi_handle) {
  return 0;
}


int process_midi(void *zcard_plugin, uint8_t *midi_message, size_t size) {
  return 0;
}


int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  return 0;
}

