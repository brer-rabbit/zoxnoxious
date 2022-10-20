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
#include "card.h"

#define DEFAULT_ZOXNOXIOUS_DIRECTORY "/usr/local/zoxnoxiousd"
#define ZOXNOXIOUS_DIR_ENV_VAR_NAME "ZOXNOXIOUS_DIR"
#define CONFIG_DIRNAME "/etc/"
#define CONFIG_FILENAME "zoxnoxiousd.cfg"


void sig_cleanup_and_exit(int signum) {
  // log and close anything relevant
  exit(0);
}


static void help() {
  printf("Usage: zoxnoxiousd ...\n");
}


struct alsa_pcm_state {
  snd_pcm_t *pcm_handle;
  unsigned int sampling_rate;
  snd_pcm_sframes_t period_size;  // Size to request on read()
  snd_pcm_uframes_t buffer_size;  // size of ALSA buffer (in frames)
  snd_pcm_format_t format;        // audiobuf format
  unsigned int channels;          // number of channels
  int first_period;               // boolean cleared after first frame processed, set after xrun
};


/* Config keys */
char *config_lookup_eeprom_base_i2c_address = "eeprom_base_i2c_address";


/* zlog loggin */
zlog_category_t *zlog_c = NULL;



int main(int argc, char **argv, char **envp) {
  config_t *cfg;
  char *audio_device1_name = NULL, *audio_device2_name = NULL, *midi_device_name = NULL;
  char config_filename[128] = { '\0' };

  char *opt_string = "hi:d:e:m:v";

  struct option long_option[] = {
    {"help", no_argument, NULL, 'h'},
    {"config", required_argument, NULL, 'i'},
    {"device1", required_argument, NULL, 'd'},
    {"device2", required_argument, NULL, 'e'},
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
    case 'd':
      audio_device1_name = strdup(optarg);
      break;
    case 'e':
      audio_device2_name = strdup(optarg);
      break;
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
  if (zlog_init("/home/kaf/git/zoxnoxious/raspberry_pi/etc/zlog.cfg")) {
    printf("zlog_init failed");
    return -1;
  }

  zlog_c = zlog_get_category("zoxnoxious");
  if (!zlog_c) {
    printf("get zlog cat failed\n");
    zlog_fini();
    return -1;
  }

  cfg = (config_t*)malloc(sizeof(config_t));
  config_init(cfg);


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
         "  audio_device1: %s\n"
         "  audio_device2: %s\n"
         "  midi_device: %s\n",
         config_filename,
         audio_device1_name ? audio_device1_name : "(null)",
         audio_device2_name ? audio_device2_name : "(null)",
         midi_device_name ? midi_device_name : "(null)");

  if (! config_read_file(cfg, config_filename)) {
    printf("config read failed\n");
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


  /* detect installed cards */
  struct plugin_card *plugin_cards;
  int num_cards;
  int i2c_base_address;
  config_lookup_int(cfg, config_lookup_eeprom_base_i2c_address, &i2c_base_address);
  plugin_cards = discover_cards(i2c_base_address, &num_cards);


  /* load and initialize plugins */


  /* init alsa */






  zlog_fini();
  config_destroy(cfg);
  free(cfg);
  free(audio_device1_name);
  free(audio_device2_name);
  free(midi_device_name);



  return 0;
}
