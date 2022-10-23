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

/* main file for zoxnoxiousd server application.  Handle basic setup of components,
 * init, get things going.
 */

#include <alsa/asoundlib.h>
#include <getopt.h>
#include <libconfig.h>
#include <pigpio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <zlog.h>

#include "zoxnoxiousd.h"
#include "card_manager.h"
#include "zalsa.h"



static void help() {
  printf("Usage: zoxnoxiousd <options>\n"
         "  -i <config_file>\n"
}




/* Config keys */
/* When needed go here */



/* zlog loggin' */
zlog_category_t *zlog_c = NULL;

/* globals-  mainly so they can be accessed by signal handler  */
struct card_manager *card_mgr = NULL;
struct alsa_pcm_state *pcm_state[2] = { NULL, NULL };



void sig_cleanup_and_exit(int signum) {
  // log and close anything relevant
  exit(0);
}


int main(int argc, char **argv, char **envp) {
  config_t *cfg;
  char *midi_device_name = NULL;
  char config_filename[128] = { '\0' };
  char *opt_string = "hi:m:v";

  struct option long_option[] = {
    {"help", no_argument, NULL, 'h'},
    {"config", required_argument, NULL, 'i'},
    {"mididevice", required_argument, NULL, 'm'},
    {"verbose", required_argument, NULL, 'v'},
    {NULL, 0, NULL, 0},
  };


  // parse command line
  while (1) {
    int c;
    if ((c = getopt_long(argc, argv, opt_string, long_option, NULL)) < 0) {
      break;
    }
    switch (c) {
    case 'h':
      help();
      return 1;
    case 'i':
      strncpy(config_filename, optarg, sizeof(config_filename) - 1);
      config_filename[127] = '\0';
      break;
    case 'm':
      midi_device_name = strdup(optarg);
      break;
    default:
      printf("unknown option\n");
      help();
      return -1;
    }
  }


  // TODO: hardcoded directory
  if (zlog_init("/home/kaf/git/zoxnoxious/raspberry_pi/etc/log.cfg")) {
    printf("zlog_init failed");
    return -1;
  }

  zlog_c = zlog_get_category("zoxnoxious");
  if (!zlog_c) {
    printf("get zlog cat failed\n");
    zlog_fini();
    return -1;
  }


  if (config_filename[0] != '\0') {
    INFO("using config %s\n", config_filename);
  }
  else if (getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME) != NULL) {
    snprintf(config_filename, 128, "%s%s%s",
             getenv(ZOXNOXIOUS_DIR_ENV_VAR_NAME), CONFIG_DIRNAME, CONFIG_FILENAME);
  }
  else {
    snprintf(config_filename, 128, "%s%s%s",
             DEFAULT_ZOXNOXIOUS_DIRECTORY, CONFIG_DIRNAME, CONFIG_FILENAME);
  }

  printf("params:\n"
         "  config filename: %s\n"
         "  midi_device: %s\n",
         config_filename,
         midi_device_name ? midi_device_name : "(null)");


  cfg = (config_t*)malloc(sizeof(config_t));
  config_init(cfg);

  if (config_read_file(cfg, config_filename) == CONFIG_FALSE) {
    ERROR("config read failed to read file %s: %s (%d)", config_filename, config_error_text(cfg), config_error_type(cfg));
    return -1;
  }


  // SPI, pigpio start
  if (gpioInitialise() < 0) {
    ERROR("gpioInitialise failed, bye!");
    return -1;
  }
  else {
    INFO("gpioInitialise complete");
  }


  /* Basic initialization done:
   * + libconf opened, available
   * + zlog enabled, logging available
   * + pigpio init, can start talking to hardware
   *
   * what's not done:
   * - load card plugins
   * - alsa init
   */


  // detect installed cards- get the card manager going
  card_mgr = init_card_manager(cfg);
  discover_cards(card_mgr);
  load_card_plugins(card_mgr);


  // init alsa pcm devices
  pcm_state[0] = init_alsa_device(cfg, 0);

  // only init the second if the first is valid
  if (pcm_state[0]) {
    INFO("pcm initialized for %s", pcm_state[0]->device_name);

    pcm_state[1] = init_alsa_device(cfg, 1);

    if (pcm_state[1]) {
      INFO("pcm initialized for %s", pcm_state[1]->device_name);
    }

  }


  // init alsa midi device


  // assign channels


  // setup signal handling


  // start threads


  zlog_fini();
  config_destroy(cfg);
  free(cfg);

  return 0;
}
