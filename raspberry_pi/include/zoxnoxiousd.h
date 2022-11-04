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

#include <zlog.h>

#include "zcard_plugin.h"


#define DEBUG(...) zlog_debug(zlog_c, __VA_ARGS__)
#define INFO(...)  zlog_info(zlog_c, __VA_ARGS__)
#define WARN(...)  zlog_warn(zlog_c, __VA_ARGS__)
#define ERROR(...) zlog_error(zlog_c, __VA_ARGS__)
#define FATAL(...) zlog_fatal(zlog_c, __VA_ARGS__)
extern zlog_category_t *zlog_c;

#define ZOXNOXIOUS_DIR_ENV_VAR_NAME "ZOXNOXIOUS_DIR"
#define DEFAULT_ZOXNOXIOUS_DIRECTORY "/usr/local/zoxnoxious"
#define CONFIG_DIRNAME "/etc/"
#define CONFIG_FILENAME "zoxnoxiousd.cfg"

#define MIDI_DEVICE_KEY "zmidi.device"

#endif
