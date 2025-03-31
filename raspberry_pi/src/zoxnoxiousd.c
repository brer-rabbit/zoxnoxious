/* Copyright 2023 Kyle Farrell
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
 * run it something like this with LD_LIBRARY_PATH set to /usr/local/zoxnoxiousd/lib
 * sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH chrt -f 40 ./zoxnoxiousd -i /home/kaf/git/zoxnoxious/raspberry_pi/etc/zoxnoxiousd.cfg
 */

#include <alsa/asoundlib.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <pigpio.h>
#include <poll.h>
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
#include "tune_mgr.h"
#include "card_manager.h"
#include "zalsa.h"
#include "zcard_plugin.h"


// number of stats to track and what they mean
#define NUM_MISSED_EXPIRATIONS_STATS 1024
#define EXPIRATIONS_ONTIME 0
#define EXPIRATIONS_MISSED_ONE 1
#define EXPIRATIONS_MISSED_LT_TEN 2
#define EXPIRATIONS_MISSED_GTE_TEN 3
#define DISCOVERY_REPORT_SIZE_BYTES 28

// midi thread polls with a timeout to check for thread termination condition
#define MIDI_TIMEOUT_MS 10000

/* globals-  mainly so they can be accessed by signal handler  */
static struct card_manager *card_mgr = NULL;
static struct alsa_pcm_state *pcm_state[2] = { NULL, NULL };
static snd_rawmidi_t *midi_in = NULL;
static snd_rawmidi_t *midi_out = NULL;
static pthread_mutex_t midi_out_mutex = PTHREAD_MUTEX_INITIALIZER;

static _Atomic int alsa_thread_run = 1;
static _Atomic uint64_t missed_expirations[NUM_MISSED_EXPIRATIONS_STATS] = { 0 };
static _Atomic time_t sec_pcm_write_idle = 0;
static _Atomic long nsec_pcm_write_idle = 0;

static _Atomic int system_tune_requested = 0;
static _Atomic int system_tune_in_progress = 0;


// static functions
static void help();
static void sig_cleanup_and_exit(int signum);
static void sig_dump_stats(int signum);
static int open_midi_device(config_t *cfg);
static void* read_pcm_and_call_plugins(void *);
static void* midi_in_to_plugins(void *);
static void generate_discovery_report(uint8_t discovery_report_sysex[]);
static int z_midi_write(uint8_t *buffer, int buffer_size);
static int get_midi_input_fd();


int main(int argc, char **argv, char **envp) {
  config_t *cfg;
  char *midi_device_name = NULL;
  char config_filename[128] = { '\0' };
  char *opt_string = "hi:v";
  pthread_t alsa_pcm_to_plugin_thread;
  pthread_t midi_in_plugin_thread;
  sigset_t signal_set;
  struct sigaction signal_action_cleanup, signal_action_stats;


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
  gpioCfgClock(4, 1, 1);
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
  if (open_midi_device(cfg) != 0) {
    ERROR("fail to open MIDI device");
    abort();
  }

  
  // detect installed cards- get the card manager going
  card_mgr = init_card_manager(cfg);
  discover_cards(card_mgr);
  load_card_plugins(card_mgr);
  assign_update_order(card_mgr);
  assign_hw_audio_channels(card_mgr, num_hw_channels, 2);

  struct zhost *zhost;
  if ( (zhost = zhost_create()) == NULL) {
    FATAL("zhost_create failed");
    abort();
  }

  // init all the plugin cards
  for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
    // alias
    struct plugin_card *this_card = card_mgr->card_update_order[card_num];
    // the index isn't the slot num-- but we can look it up on the card
    INFO("init card slot %d", card_mgr->cards[card_num].slot);
    this_card->plugin_object =
      (this_card->init_zcard)(zhost, card_mgr->cards[card_num].slot);
    if (this_card->plugin_object == NULL) {
      WARN("plugin card slot %d returned NULL for init", card_mgr->cards[card_num].slot);
    }
  }



  sigemptyset(&signal_set);
  sigaddset(&signal_set, SIGUSR1);
  sigaddset(&signal_set, SIGHUP);
  sigaddset(&signal_set, SIGINT);
  sigaddset(&signal_set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &signal_set, NULL);


  // cpu tune request for all cards:
  // this isn't done at startup so one can tweak trimmers before autotuning.
  // future: give user option via frontend to use linear or corrected tables.
  //autotune_all_cards(card_mgr);

  // start threads
  if ( pthread_create(&alsa_pcm_to_plugin_thread, NULL, read_pcm_and_call_plugins, NULL) ) {
    ERROR("failed to start thread for read_pcm_and_call_plugins");
    abort();
  }

  if ( pthread_create(&midi_in_plugin_thread, NULL, midi_in_to_plugins, NULL) ) {
    ERROR("failed to start thread for midi_in_to_plugins");
    abort();
  }


  // setup signal handling
  signal_action_cleanup.sa_handler = sig_cleanup_and_exit;
  sigaction(SIGHUP, &signal_action_cleanup, NULL);
  sigaction(SIGINT, &signal_action_cleanup, NULL);
  sigaction(SIGTERM, &signal_action_cleanup, NULL);

  signal_action_stats.sa_handler = sig_dump_stats;
  sigaction(SIGUSR1, &signal_action_stats, NULL);

  while (alsa_thread_run) {
    sleep(1);
  }

  int retval;
  pthread_join(alsa_pcm_to_plugin_thread, (void**)&retval);
  pthread_join(midi_in_plugin_thread, (void**)&retval);

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


  // fall through to exit
  zlog_fini();
  config_destroy(cfg);
  free(cfg);

  return 0;
}




static void help() {
  printf("Usage: zoxnoxiousd <options>\n"
         "  -i <config_file>\n");
}


// Signal handling
static volatile sig_atomic_t in_aborting = 0;

// this isn't a very safe signal handler...
void sig_cleanup_and_exit(int signum) {
  if (in_aborting) {
    return;
  }
  in_aborting = 1;

  // message threads to abort
  alsa_thread_run = 0;
}


static volatile sig_atomic_t in_dump_stats = 0;
static void sig_dump_stats(int signum) {
  if (in_dump_stats) {
    return;
  }
  in_dump_stats = 1;

  INFO("requested stats: %ld.%.9ld / %" PRId64 "; pcm[0] xrun recovery: %d; pcm[1] xrun recovery: %d",
       sec_pcm_write_idle, nsec_pcm_write_idle,
       missed_expirations[EXPIRATIONS_ONTIME],
       pcm_state[0] ? pcm_state[0]->xrun_recovery_count : -1,
       pcm_state[1] ? pcm_state[1]->xrun_recovery_count : -1);

  for (int i = 1; i < NUM_MISSED_EXPIRATIONS_STATS; ++i) {
    if (missed_expirations[i]) {
      INFO("  missed %d expirations %" PRId64 " times", i, missed_expirations[i]);
    }
  }

    if (missed_expirations[NUM_MISSED_EXPIRATIONS_STATS -1]) {
      INFO("  missed at least %d expirations %" PRId64 " times", NUM_MISSED_EXPIRATIONS_STATS - 1, missed_expirations[NUM_MISSED_EXPIRATIONS_STATS -1]);
    }

  in_dump_stats = 0;
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


// add timespec in t1 to accumulator storing in accumulator
static inline void timespec_accumulate(const struct timespec *t1, struct timespec *accumulator) {
  accumulator->tv_sec += t1->tv_sec;
  accumulator->tv_nsec += t1->tv_nsec;
  if (accumulator->tv_nsec >= 1000000000) {
    accumulator->tv_nsec -= 1000000000;
    accumulator->tv_sec++;
  }
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

static void* read_pcm_and_call_plugins(void *arg) {
  int timerfd_sample_clock;
  uint64_t expirations = 0;
  int frames_to_advance;

  // do a couple dumb things:
  // compute timer dynamically... but this is really designed
  // for 4khz.  Even worse, assume that pcm[0] and [1] have
  // the same sampling rate.
  struct itimerspec itimerspec_sample_clock = {
    .it_interval.tv_sec = 0,
    .it_interval.tv_nsec = 1000000000 / pcm_state[0]->sampling_rate,
    .it_value.tv_sec = 0,
    .it_value.tv_nsec = 1000000000 / pcm_state[0]->sampling_rate,
  };
  struct itimerspec itimerspec_remaining_time;
  struct timespec accumulated_idle_time = { 0 };
  int valid_gettime;


  if ( (timerfd_sample_clock = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
    char error[256];
    strerror_r(errno, error, 256);
    ERROR("failed to create sample clock timer: %s", error);
  }

  INFO("starting timer %ld usec for sampling rate %d hz",
       itimerspec_sample_clock.it_interval.tv_nsec / 1000,
       pcm_state[0]->sampling_rate);
  // logging from here forward may be tricky / time sensitive

  if ( alsa_start_stream(pcm_state[0]) ) {
    ERROR("pcm0: error from alsa_pcm_ensure_ready");
  }

  if (pcm_state[1]) {
    if ( alsa_start_stream(pcm_state[1]) ) {
      ERROR("pcm1: error from alsa_pcm_ensure_ready");
    }
  }


  if ( (timerfd_settime(timerfd_sample_clock, 0, &itimerspec_sample_clock, 0) ) == -1) {
    char error[256];
    strerror_r(errno, error, 256);
    ERROR("failed to start timer: %s", error);
  }


  while (alsa_thread_run) {

    // Business Section
    for (int card_num = 0; card_num < card_mgr->num_cards; ++card_num) {
      // alias for the deeply nested structure to the plugin card / readability
      struct plugin_card *plugin_card = card_mgr->card_update_order[card_num];
      int channel_offset = plugin_card->channel_offset;

      // the samples relevant for this card are at the channel offset on the approp pcm device
      const int16_t *samples = (const int16_t*) ( plugin_card->pcm_device_num == 0 ?
                                                  pcm_state[0]->samples[channel_offset] : pcm_state[1]->samples[channel_offset] );

      // then call the card's plugin with the samples via function pointer
      if ( (plugin_card->process_samples)(plugin_card->plugin_object, samples) != 0) {
        INFO("card error");
      }
    }


    if (system_tune_requested) {
      system_tune_in_progress = 1;
      INFO("MIDI tune starting");
      autotune_all_cards(card_mgr);
      system_tune_in_progress = 0;
      system_tune_requested = 0;
    }

    // check on remaining time-- though we don't know if it's remaining time until we check the expirations
    valid_gettime = timerfd_gettime(timerfd_sample_clock, &itimerspec_remaining_time);
    read(timerfd_sample_clock, &expirations, sizeof(expirations));

    if (expirations == 1) {
      missed_expirations[EXPIRATIONS_ONTIME]++;
      // the gettime ended up being remaining time
      if (valid_gettime == 0) {
        timespec_accumulate(&itimerspec_remaining_time.it_value, &accumulated_idle_time);
        sec_pcm_write_idle = accumulated_idle_time.tv_sec;
        nsec_pcm_write_idle = accumulated_idle_time.tv_nsec;
      }
      else {
        WARN("timerfd_gettime returned %d", valid_gettime);
      }
    }
    else if (expirations < NUM_MISSED_EXPIRATIONS_STATS - 1) {
      missed_expirations[expirations]++;
    }
    else {
      missed_expirations[NUM_MISSED_EXPIRATIONS_STATS - 1]++;
    }

    // downcast
    frames_to_advance = expirations > INT_MAX ? INT_MAX : expirations;

    // get new set of frames or advance sample pointers
    if (pcm_state[1]) {
      int pcm1_return = alsa_advance_stream_by_frames(pcm_state[1], frames_to_advance);
      if (pcm1_return) {
        INFO("pcm1: alsa_advance_stream_by_frames: %d", pcm1_return);
      }
    }

    int pcm0_return = alsa_advance_stream_by_frames(pcm_state[0], frames_to_advance);
    if (pcm0_return) {
      INFO("pcm0: alsa_advance_stream_by_frames: %d", pcm0_return);
    }

  }

  INFO("stats: %" PRId64 " frames @ %" PRId64 " idle usec/frame; %" PRId64 " one-miss; %" PRId64 " less than ten; %" PRId64 " ten or more missed expirations",
       missed_expirations[EXPIRATIONS_ONTIME],       
       (((int64_t)sec_pcm_write_idle * 1000000000LL + nsec_pcm_write_idle) / 1000LL) / ((int64_t)missed_expirations[EXPIRATIONS_ONTIME]),
       missed_expirations[EXPIRATIONS_MISSED_ONE],
       missed_expirations[EXPIRATIONS_MISSED_LT_TEN],
       missed_expirations[EXPIRATIONS_MISSED_GTE_TEN]);

  return NULL;
}



// Stuff for midi_in_to_plugins.  Parse the midi stream and determine what to send to the cards.

// this is all the first nibble stuff- 0xFx stuff is more complex, but it's to be ignored
enum midi_status_byte {
  MIDI_STATUS_NOT_SET = 0x00,
  MIDI_NOTE_OFF = 0x80,
  MIDI_NOTE_ON = 0x90,
  MIDI_KEY_PRESSURE = 0xA0,
  MIDI_CONTROLLER_CHANGE = 0xB0,
  MIDI_PROGRAM_CHANGE = 0xC0,
  MIDI_CHANNEL_PRESSURE = 0xD0,
  MIDI_PITCH_BEND = 0xE0,
  MIDI_SYSEX_START = 0xF0,
  MIDI_TUNE_REQUEST = 0xF6,
  MIDI_SYSEX_END = 0xF7
};

enum sysex_message_type {
  TYPE_UNSET = 0,
  DISCOVERY_RESPONSE = 0x01,
  DISCOVERY_REQUEST = 0x02,
  SHUTDOWN_REQUEST = 0x03,
  RESTART_REQUEST = 0x04,
  VALID_MANUFACTURER_ID = 0x7D
};

struct midi_state {
  enum midi_status_byte status;
  uint8_t channel;
  enum sysex_message_type sysex_message_type;
  int sysex_buffer_size;
  uint8_t sysex_buffer[32];
};

// Discovered card number maps to MIDI channel number.  Not actual slot number.


/* get_midi_input_fd gets the midi file descriptor such that it can be polled for input
 * Return <0 on error.
 */
static int get_midi_input_fd() {
  int midi_fd = -1;
  int num_descriptors = snd_rawmidi_poll_descriptors_count(midi_in);

  if (num_descriptors <= 0) {
    ERROR("Error: No poll descriptors available for MIDI input.");
    return -1;
  }

  // initialization only- I'll allow for calloc here
  struct pollfd *pfds = calloc(num_descriptors, sizeof(struct pollfd));
  if (!pfds) {
    ERROR("Error allocating memory for MIDI poll descriptors");
    return -1; // Indicate error
  }

  if (snd_rawmidi_poll_descriptors(midi_in, pfds, num_descriptors) > 0) {
    // Assuming only one readable descriptor for MIDI input
    for (int i = 0; i < num_descriptors; ++i) {
      if (pfds[i].events & POLLIN) {
        midi_fd = pfds[i].fd;
        break;
      }
    }
    if (midi_fd == -1) {
      ERROR("Error: Could not find a readable MIDI input descriptor");
    }
  }
  else {
    ERROR("Error: Failed to get MIDI poll descriptors: %s", snd_strerror(snd_rawmidi_poll_descriptors(midi_in, pfds, num_descriptors)));
  }

  free(pfds);
  return midi_fd;
}


// read the midi stream, pass along to cards.  To be called as a thread.
// Poll the MIDI in stream with a timeout of MIDI_TIMEOUT_MS.  Timeout allows
// periodic checking of whether the thread should terminate or not.
static void* midi_in_to_plugins(void *arg) {
  int midi_fd = get_midi_input_fd();
  int midi_read_status = 0;
  uint8_t buffer[256];
  struct midi_state midi_state = { 0 };
  uint8_t discovery_report_sysex[DISCOVERY_REPORT_SIZE_BYTES] = { 0 };

  if (midi_fd < 0) {
    ERROR("Failed to obtain MIDI input file descriptor. MIDI input will not be processed.");
    return NULL; // Or handle how exactly?
  }

  generate_discovery_report(discovery_report_sysex);

  INFO("starting MIDI In event loop");

  while (alsa_thread_run) {

    struct pollfd fds[1];
    fds[0].fd = midi_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    int retval = poll(fds, 1, MIDI_TIMEOUT_MS);

    if (retval == -1) {
      if (errno == EINTR) {
        continue;
      }
      else {
        ERROR("Error in poll(): %s", strerror(errno));
        break;
      }
    }
    else if (retval > 0) {
      if (fds[0].revents & POLLIN) {

        midi_read_status = snd_rawmidi_read(midi_in, buffer, sizeof(buffer));
        if (midi_read_status > 0) {
          INFO("MIDI read received %d bytes", midi_read_status);

          // read the midi stream. could be starting anywhere in the stream,
          // account for that by tracking stream state. This makes the stream parsing a
          // bit obtuse but we need to account for fragmented reads and such.
          for (int i = 0; i < midi_read_status; ++i) {

            if ( (buffer[i] & 0xF0) == MIDI_PROGRAM_CHANGE) {
              midi_state.status = MIDI_PROGRAM_CHANGE;
              midi_state.channel = buffer[i] & 0x0F;
            }
            else if (midi_state.status == MIDI_PROGRAM_CHANGE) {
              // get program change and call plugin
              INFO("MIDI: channel 0x%X program change: 0x%X",
                   midi_state.channel,
                   buffer[i]);
              // card numbering maps to midi channel.  So check we've got a valid
              // midi channel against how many cards we've got to ensure we can
              // dispatch the midi message.
              if (midi_state.channel < card_mgr->num_cards) {
                struct plugin_card *card = &card_mgr->cards[ midi_state.channel ];
                // leap of faith into the function
                card->process_midi_program_change(card->plugin_object, buffer[i]);
              }
              else {
                WARN("Expected midi message on channel 0x%X to map to a user card",
                     midi_state.channel);
              }

              // then clear state
              midi_state.status = MIDI_STATUS_NOT_SET;
            }
            else if (buffer[i] == MIDI_SYSEX_START) {  // no channel for sysex
              midi_state.status = MIDI_SYSEX_START;
              INFO("MIDI: sysex start received");
              midi_state.channel = 0;
              midi_state.sysex_message_type = TYPE_UNSET;
              midi_state.sysex_buffer_size = 0;
            }
            else if (midi_state.status == MIDI_SYSEX_START &&
                     midi_state.sysex_message_type == TYPE_UNSET) {
              if (buffer[i] == VALID_MANUFACTURER_ID) {
                midi_state.sysex_message_type = VALID_MANUFACTURER_ID;
                INFO("MIDI: sysex manufacturer message correct");
              }
              else {
                midi_state.status = MIDI_STATUS_NOT_SET;
              }
            }
            else if (midi_state.status == MIDI_SYSEX_START &&
                     midi_state.sysex_message_type == VALID_MANUFACTURER_ID) {
              if (buffer[i] == DISCOVERY_REQUEST) {
                // no additional data required for a discovery request
                // action is to send a discovery response
                INFO("MIDI: discovery sysex request received, sending %d bytes",
                     sizeof(discovery_report_sysex) / sizeof(uint8_t));
                z_midi_write(discovery_report_sysex, sizeof(discovery_report_sysex) / sizeof(uint8_t));
              }
              else if (buffer[i] == SHUTDOWN_REQUEST) {
                INFO("Shutdown request received");
                system("sudo poweroff --poweroff");
              }
              else if (buffer[i] == RESTART_REQUEST) {
                INFO("Restart request received");
                system("sudo reboot --reboot");
              }
              else {
                INFO("MIDI: sysex unknown request received");
              }

              midi_state.status = MIDI_STATUS_NOT_SET;
            }
            else if (buffer[i] == MIDI_TUNE_REQUEST) {  // no channel for sysex
              INFO("MIDI tune requested");
              if (!system_tune_in_progress) {
                system_tune_requested = 1;
              }
            }
            // future: check for other status messages
          }
        }
        else if (midi_read_status < 0 && midi_read_status != -EAGAIN) {
          ERROR("Error reading MIDI input: %s", snd_strerror(midi_read_status));
          break;
        }
      }
    } // else - Can do other non-MIDI related tasks here if needed
  }
  INFO("Exiting MIDI input thread.");
  return NULL;
}


// simple function to produce the static discovery report in midi sysex.
// Assumes that 28 bytes are available in the buffer.
// Discovery report spec is documented in the "midi.spec" file.
static void generate_discovery_report(uint8_t discovery_report_sysex[]) {
  discovery_report_sysex[0] = 0xF0; // sysex start
  discovery_report_sysex[1] = 0x7D; // test manufacturer
  discovery_report_sysex[2] = 0x01; // sysex discovery report
  discovery_report_sysex[27] = 0xF7; // end sysex

  for (int i = 0; i < card_mgr->num_cards; ++i) {
    discovery_report_sysex[3 + card_mgr->cards[i].slot * 3] = card_mgr->cards[i].card_id;
    discovery_report_sysex[4 + card_mgr->cards[i].slot * 3] = card_mgr->cards[i].channel_offset;
    discovery_report_sysex[5 + card_mgr->cards[i].slot * 3] = card_mgr->cards[i].pcm_device_num;
  }

}



// z_midi_write
static int z_midi_write(uint8_t *buffer, int buffer_size) {
  int midi_write_status = -1;
  pthread_mutex_lock(&midi_out_mutex);
  if (midi_out != NULL) {
    midi_write_status = snd_rawmidi_write(midi_out, buffer, buffer_size);
  }
  else {
    ERROR("MIDI: midi out is NULL");
  }

  pthread_mutex_unlock(&midi_out_mutex);

  if (midi_write_status < 0) {
    ERROR("Error writing to midi output: %s", snd_strerror(midi_write_status));
  }
  else if (midi_write_status != buffer_size) {
    ERROR("Error writing to midi output: wrote %d of %d bytes",
          midi_write_status, buffer_size);
  }

  return midi_write_status;
}
