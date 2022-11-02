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
#include <sys/timerfd.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <zlog.h>

#include "zoxnoxiousd.h"
#include "card_manager.h"
#include "zalsa.h"


// number of stats to track and what they mean
#define NUM_MISSED_EXPIRATIONS_STATS 4
#define EXPIRATIONS_ONTIME 0
#define EXPIRATIONS_MISSED_ONE 1
#define EXPIRATIONS_MISSED_LT_TEN 2
#define EXPIRATIONS_MISSED_GTE_TEN 3

static void help() {
  printf("Usage: zoxnoxiousd <options>\n"
         "  -i <config_file>\n");
}




/* Config keys */
/* When needed go here */



/* zlog loggin' */
zlog_category_t *zlog_c = NULL;

/* globals-  mainly so they can be accessed by signal handler  */
struct card_manager *card_mgr = NULL;
struct alsa_pcm_state *pcm_state[2] = { NULL, NULL };
static snd_rawmidi_t *midi_in = NULL;
static snd_rawmidi_t *midi_out = NULL;


// static functions
static void sig_cleanup_and_exit(int signum);
static int open_midi_device(config_t *cfg);
static void read_pcm_and_call_plugins(struct card_manager *card_mgr, struct alsa_pcm_state *pcm_state[]);




int main(int argc, char **argv, char **envp) {
  config_t *cfg;
  char *midi_device_name = NULL;
  char config_filename[128] = { '\0' };
  char *opt_string = "hi:m:v";

  /* bookkeeping stuff before getting to the important stuff:
   * + libconf opened, available
   * + zlog enabled, logging
   * + pigpio init, can start talking to hardware
   * + load card plugins
   * + midi opened
   * + alsa init
   */


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
#ifndef MOCK_DATA
  if (gpioInitialise() < 0) {
    ERROR("gpioInitialise failed, bye!");
    return -1;
  }
  else {
    INFO("gpioInitialise complete");
  }
#endif



  // init alsa pcm devices
  pcm_state[0] = init_alsa_device(cfg, 0);
  int num_hw_channels[2] = { 0 };

  // only init the second if the first is valid
  if (pcm_state[0]) {
    INFO("pcm initialized for %s", pcm_state[0]->device_name);
    num_hw_channels[0] = pcm_state[0]->channels;

    pcm_state[1] = init_alsa_device(cfg, 1);

    if (pcm_state[1]) {
      INFO("pcm initialized for %s", pcm_state[1]->device_name);
      num_hw_channels[1] = pcm_state[1]->channels;
    }

    if (pcm_state[1] && pcm_state[0]->channels != pcm_state[1]->channels) {
      FATAL("devices must have same number of channels: %s (%d) and %s (%d)",
            pcm_state[0]->device_name, pcm_state[0]->channels,
            pcm_state[1]->device_name, pcm_state[1]->channels);
      abort();
    }
  }


  // init alsa midi device
  open_midi_device(cfg);

  
  // detect installed cards- get the card manager going
  card_mgr = init_card_manager(cfg);
  discover_cards(card_mgr);
  load_card_plugins(card_mgr);
  assign_update_order(card_mgr);
  assign_hw_audio_channels(card_mgr, num_hw_channels, 2);


  // setup signal handling
  signal(SIGHUP, sig_cleanup_and_exit);
  signal(SIGINT, sig_cleanup_and_exit);
  signal(SIGTERM, sig_cleanup_and_exit);


  // start threads
  read_pcm_and_call_plugins(card_mgr, pcm_state);



  // fall through to exit
  zlog_fini();
  config_destroy(cfg);
  free(cfg);

  return 0;
}





// Signal handling
static volatile sig_atomic_t in_aborting = 0;

void sig_cleanup_and_exit(int signum) {
  if (in_aborting) {
    return;
  }
  in_aborting = 1;


  // message threads to abort


  // close pcm handles
  if (pcm_state[0] && pcm_state[0]->pcm_handle) {
    snd_pcm_abort(pcm_state[0]->pcm_handle);
    snd_pcm_close(pcm_state[0]->pcm_handle);
  }

  if (pcm_state[1] && pcm_state[1]->pcm_handle) {
    snd_pcm_abort(pcm_state[1]->pcm_handle);
    snd_pcm_close(pcm_state[1]->pcm_handle);
  }

  // card mgr closes all plugins
  if (card_mgr) {
    free_card_manager(card_mgr);
  }

  gpioTerminate();

  // log and close anything relevant
  exit(0);
}



static int open_midi_device(config_t *cfg) {
  const char *midi_device_name;
  config_setting_t *midi_device_setting = config_lookup(cfg, MIDI_DEVICE_KEY);
    
  if (midi_device_setting == NULL) {
    ERROR("cfg: no midi config value found for " MIDI_DEVICE_KEY);
    return 1;
  }

  midi_device_name = config_setting_get_string(midi_device_setting);

  if (midi_device_name == NULL) {
    ERROR("cfg: device name null for key " MIDI_DEVICE_KEY);
    return 1;
  }

  if (snd_rawmidi_open(&midi_in, &midi_out, midi_device_name, SND_RAWMIDI_NONBLOCK) != 0) {
    ERROR("failed to open midi device %s", midi_device_name);
    return 1;
  }

  INFO("successfully opened midi device %s", midi_device_name);
  return 0;
}



// start timer
// forever:
// foreach pcm stream
//   alsa_pcm_ensure_ready
//   snd_pcm_mmap_begin
//   calc samples[channelnum]
// 
// do {
//   call each plugin on stream 1
//   if stream 2 call each plugin on stream 1
//   read timer
//   advance samples pointer
//   if (!frames stream 1): commit, ensure ready, mmap
//   if (!frames stream 2): commit, ensure ready, mmap
// } while (running flag)

static void read_pcm_and_call_plugins(struct card_manager *card_mgr, struct alsa_pcm_state *pcm_state[]) {
  int timerfd_sample_clock;
  uint64_t expirations = 0;
  _Atomic uint64_t missed_expirations[NUM_MISSED_EXPIRATIONS_STATS] = { 0 };
  int sample_advances = 0;
  int new_periods = 0;

  struct itimerspec itimerspec_sample_clock =
    {
      .it_interval.tv_sec = 0,
      .it_interval.tv_nsec = 1000000000 / pcm_state[0]->sampling_rate,
      .it_value.tv_sec = 0,
      .it_value.tv_nsec = 1000000000 / pcm_state[0]->sampling_rate,
    };

  if ( (timerfd_sample_clock = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
    char error[256];
    strerror_r(errno, error, 256);
    ERROR("failed to create timer: %s", error);
  }
  

  // logging from here forward may be tricky / time sensitive
  INFO("starting timer %ld usec for sampling rate %d hz",
       itimerspec_sample_clock.it_interval.tv_nsec / 1000,
       pcm_state[0]->sampling_rate);


  if ( alsa_pcm_ensure_ready(pcm_state[0]) ) {
    ERROR("pcm0: error from alsa_pcm_ensure_ready");
  }

  if (pcm_state[1]) {
    if ( alsa_pcm_ensure_ready(pcm_state[1]) ) {
      ERROR("pcm1: error from alsa_pcm_ensure_ready");
    }
  }

  alsa_mmap_begin_with_step_calc(pcm_state[0]);
  if (pcm_state[1]) {
    alsa_mmap_begin_with_step_calc(pcm_state[1]);
  }


  if ( (timerfd_settime(timerfd_sample_clock, 0, &itimerspec_sample_clock, 0) ) == -1) {
    char error[256];
    strerror_r(errno, error, 256);
    ERROR("failed to start timer: %s", error);
  }



  for (int blah = 0; blah < 30000; ++blah) {

    // Business Section
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      // alias a couple big redirects here:
      // lookup the card offset
      int channel_offset = card_mgr->card_update_order[card_num]->channel_offset;

      // the samples relevant for this card are at the offset on the approp pcm device
      const int16_t *samples = (const int16_t*) ( card_mgr->card_update_order[card_num]->pcm_device_num == 0 ?
                                                  pcm_state[0]->samples[channel_offset] : pcm_state[1]->samples[channel_offset] );

      // then call the card's plugin with the samples via function pointer
      if ( (card_mgr->card_update_order[card_num]->process_samples)(card_mgr->card_update_order[card_num]->plugin_object, samples) != 0) {
        INFO("card error");
      }
    }


    read(timerfd_sample_clock, &expirations, sizeof(expirations));
    if (expirations == 1) {
      missed_expirations[EXPIRATIONS_ONTIME]++;
    }
    else if (expirations == 2) {
      missed_expirations[EXPIRATIONS_MISSED_ONE]++;
    }
    else if (expirations < 10) {
      missed_expirations[EXPIRATIONS_MISSED_LT_TEN]++;
    }
    else {
      missed_expirations[EXPIRATIONS_MISSED_GTE_TEN]++;
    }


    // get new set of frames or advance sample pointers
    if (pcm_state[1]) {

      if (pcm_state[1]->frames_remaining > expirations) {
        // advance sample pointer by expirations --
        // TODO: error handling should use some DSP to smooth this if expirations > 1
        for (int i = 0; i < pcm_state[1]->channels; ++i) {
          pcm_state[1]->samples[i] += (expirations * pcm_state[1]->step_size_by_channel[i]);
        }
        pcm_state[1]->frames_remaining -= expirations;
      }
      else {
        if ( alsa_mmap_end(pcm_state[1]) ) {
          ERROR("alsa_mmap_end returned non-zero");
        }

        if ( alsa_pcm_ensure_ready(pcm_state[1]) ) {
          ERROR("alsa_pcm_ensure_ready returned non-zero");
        }

        if ( alsa_mmap_begin(pcm_state[1]) ) {
          ERROR("alsa_mmap_begin returned non-zero");
        }
      }
    }


    if (pcm_state[0]->frames_remaining > expirations) {
      sample_advances++;
      for (int i = 0; i < pcm_state[0]->channels; ++i) {
        pcm_state[0]->samples[i] += (expirations * pcm_state[0]->step_size_by_channel[i]);
      }
    }
    else {
      new_periods++;
      if ( alsa_mmap_end(pcm_state[0]) ) {
        ERROR("alsa_mmap_end returned non-zero");
      }

      if ( alsa_pcm_ensure_ready(pcm_state[0]) ) {
        ERROR("alsa_pcm_ensure_ready returned non-zero");
      }

      if ( alsa_mmap_begin(pcm_state[0]) ) {
        ERROR("alsa_mmap_begin returned non-zero");
      }
    }

  }

  INFO("missed expirations: %llu ontime; %llu one-miss; %llu less than ten; %llu ten or more",
       missed_expirations[EXPIRATIONS_ONTIME],
       missed_expirations[EXPIRATIONS_MISSED_ONE],
       missed_expirations[EXPIRATIONS_MISSED_LT_TEN],
       missed_expirations[EXPIRATIONS_MISSED_GTE_TEN]);
}
