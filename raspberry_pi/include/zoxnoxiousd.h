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


#ifndef ZOXNOXIOUSD_H
#define ZOXNOXIOUSD_H

#include "zcard_plugin.h"


#define DEBUG(...) zlog_debug(zlog_c, __VA_ARGS__)
#define INFO(...)  zlog_info(zlog_c, __VA_ARGS__)
#define WARN(...)  zlog_warn(zlog_c, __VA_ARGS__)
#define ERROR(...) zlog_error(zlog_c, __VA_ARGS__)
extern zlog_category_t *zlog_c;


struct plugin_card {
  int slot;
  int card_id;
  char *plugin_name;
  // plugin interface function pointers:
  process_samples process_samples_f;
  process_midi process_midi_f;
  process_midi_program_change process_midi_program_change_f;
  free_zcard free_zcard_f;
  void *plugin_object;
};




#endif
