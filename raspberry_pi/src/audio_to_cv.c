/* 
 * audio_to_cv
 * Compile:
 * gcc -Wall -o audio_to_cv audio_to_cv.c -lasound -lpigpio
 * Run:
 * sudo audio_to_cv -d hw:CARD=UAC2Gadget,DEV=0
 *
 * a Makefile and additional functionality will be useful.
 * Right now this just listens for incoming audio and maps the first
 * six channels to board-specific DAC channels via SPI
 *
 *  Portions may be copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *  lifted from aplay.c
 */

#include <alsa/asoundlib.h>
#include <getopt.h>
#include <pigpio.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>




struct alsa_pcm_state {
  snd_pcm_t *pcm_handle;
  unsigned int sampling_rate;
  snd_pcm_sframes_t period_size;  // Size to request on read()
  snd_pcm_uframes_t buffer_size;  // size of ALSA buffer (in frames)
  snd_pcm_format_t format;        // audiobuf format
  unsigned int channels;          // number of channels
  int first_period;               // boolean cleared after first frame processed, set after xrun

  int timerfd_sample_clock;
  struct itimerspec itimerspec_sample_clock;
  int timer_running;

  int16_t previous_sample[32];
};

struct midi_state {
  int next_byte_is_program; // horrid midi stream parser for now, only id program changes
};

struct i2c_gpio_state {
  uint8_t output_state[2];
};



// map the incoming PCM channel to a DAC channel.  The DAC has 6 of 8
// channels wired up as follows, some no connects in the middle.  Map
// these to provide a contiguous interface to the PCM channels.
// 0 freq CV
// 1 Hard Sync level
// 2 no connect
// 3 PWM
// 4 Triangle VCA amount
// 5 Osc Ext VCA amount
// 6 no connect
// 7 Linear FM
static const int pcm_channel_to_dac_channel_map[] = { 0, 1, 3, 4, 5, 7 };

static int alsa_init(struct alsa_pcm_state *pcm_state, char *device);
static int alsa_pcm_read_spi_write(struct alsa_pcm_state *pcm_state, int spi_handle);
static int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state);
static void help();
static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err); 

static void alsa_midi_in_to_i2c(int i2c_handle, struct i2c_gpio_state *gpio_state, snd_rawmidi_t *midi_in, struct midi_state *midi_state);
static void set_midi_program(int i2c_handle, struct i2c_gpio_state *gpio_state, uint8_t program);


// global 'cause these handles are needed for signal handling
static struct alsa_pcm_state pcm_state = { 0 };
static snd_rawmidi_t *midi_in = NULL;
static int spi_handle = 0;
static int i2c_handle = 0;
static volatile sig_atomic_t in_aborting = 0;

// ok use globals here.  civility be damned.
static int xrun_recovery_count = 0;
static int samples_dropped = 0;
static int verbose = 0;

static void signal_handler(int sig) {
  if (in_aborting) {
    return;
  }

  in_aborting = 1;
  if (pcm_state.pcm_handle) {
    snd_pcm_abort(pcm_state.pcm_handle);
    snd_pcm_close(pcm_state.pcm_handle);
  }

  printf("close pcm\n");

  if (midi_in) {
    snd_rawmidi_close(midi_in);
    midi_in  = NULL;
  }

  printf("close midi\n");

  if (spi_handle) {
    spiClose(spi_handle);
  }

  if (i2c_handle) {
    i2cClose(i2c_handle);
  }

  printf("closed i2c\n");

  gpioTerminate();

  if (pcm_state.timerfd_sample_clock) {
    close(pcm_state.timerfd_sample_clock);
  }

  if (sig == SIGABRT) {
    /* do not call snd_pcm_close() and abort immediately */
    pcm_state.pcm_handle = NULL;
    exit(EXIT_FAILURE);
  }

  printf("xrun_recovery_count: %d\n", xrun_recovery_count);
  signal(sig, SIG_DFL);
  exit(1);
}



int main (int argc, char *argv[]) {
  struct midi_state midi_state = { 0 };
  char *midi_device = "hw:1,0";
  int err;
  char *device = "plughw:0,0";
  const int spi_rate = 20000000;
  struct i2c_gpio_state gpio_state = { .output_state = { 0x00, 0x20 } };

  struct option long_option[] = {
    {"help", no_argument, NULL, 'h'},
    {"device", required_argument, NULL, 'd'},
    {"rate", required_argument, NULL, 'r'},
    {"channels", required_argument, NULL, 'c'},
    {"buffer", required_argument, NULL, 'b'},
    {"period", required_argument, NULL, 'p'},
    {"mididevice", required_argument, NULL, 'm'},
    {"verbose", required_argument, NULL, 'v'},
    {NULL, 0, NULL, 0},
  };

  char *opt_string = "hd:r:c:b:p:m:v";

  pcm_state = (struct alsa_pcm_state const) {
    .sampling_rate = 4000,
    .period_size = 32,
    .buffer_size = 64,
    .format = SND_PCM_FORMAT_S16_LE,
    .channels = 8,
    .first_period = 1,
    .timerfd_sample_clock = timerfd_create(CLOCK_MONOTONIC, 0),
    .itimerspec_sample_clock =
      {
        .it_interval.tv_sec = 0,
        .it_interval.tv_nsec = 1000000000 / 4000,
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1000000000 / 4000
      },
    .timer_running = 0,
    .previous_sample = { 0 }
  };


  while (1) {
    int c;
    if ((c = getopt_long(argc, argv, opt_string, long_option, NULL)) < 0)
      break;
    switch (c) {
    case 'h':
      help();
      return 1;
    case 'd':
      device = strdup(optarg);
      break;
    case 'r':
      pcm_state.sampling_rate = atoi(optarg);
      pcm_state.sampling_rate = pcm_state.sampling_rate < 4000 ? 4000 : pcm_state.sampling_rate;
      pcm_state.sampling_rate = pcm_state.sampling_rate > 196000 ? 196000 : pcm_state.sampling_rate;
      break;
    case 'c':
      pcm_state.channels = atoi(optarg);
      pcm_state.channels = pcm_state.channels < 1 ? 1 : pcm_state.channels;
      pcm_state.channels = pcm_state.channels > 1024 ? 1024 : pcm_state.channels;
      break;
    case 'b':
      pcm_state.buffer_size = atoi(optarg);
      pcm_state.buffer_size = pcm_state.buffer_size < 2 ? 2 : pcm_state.buffer_size;
      pcm_state.buffer_size = pcm_state.buffer_size > 1024 ? 1024 : pcm_state.buffer_size;
      break;
    case 'p':
      pcm_state.period_size = atoi(optarg);
      pcm_state.period_size = pcm_state.period_size < 2 ? 2 : pcm_state.period_size;
      pcm_state.period_size = pcm_state.period_size > 2048 ? 2048 : pcm_state.period_size;
      break;
    case 'm':
      midi_device = strdup(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    }
  }


  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGABRT, signal_handler);


  printf("device: %s mididevice: %s\n", device, midi_device);


  // ALSA AUDIO AND MIDI INITIALIZE - create handles
  err = alsa_init(&pcm_state, device);
  if (err != 0) {
    fprintf(stderr, "alsa init failed\n");
    exit(err);
  }

  err = snd_rawmidi_open(&midi_in, NULL, midi_device, SND_RAWMIDI_NONBLOCK);
  if (err != 0) {
    fprintf(stderr, "alsa midi open failed\n");
    exit(err);
  }


  // END ALSA INIT
  // PIGPIO INIT

  int cfg = gpioCfgGetInternals();
  cfg |= PI_CFG_NOSIGHANDLER;  // (1<<10)
  gpioCfgSetInternals(cfg);
  if (gpioInitialise() < 0) {
    fprintf(stderr, "gpioInitialise failed, exiting\n");
    exit(1);
  }

  spi_handle = spiOpen(0, spi_rate, 1);
  i2c_handle = i2cOpen(1, 0x20, 0); // hardcode to PCA9555 for now

  // END PIGPIO INIT

  printf("alsa init, pigpio init: complete\n");

  //for (int i = 0; i < 400; ++i) {
  while (1) {
    alsa_pcm_read_spi_write(&pcm_state, spi_handle);
    alsa_midi_in_to_i2c(i2c_handle, &gpio_state, midi_in, &midi_state);
  }


  // if we get this far exit via the same code as the signal handler
  signal_handler(SIGTERM);

  return 0;
}




static void help() {
  printf(
         "Usage: alsa_mmap_spi [OPTION]...\n"
         "-h,--help  help\n"
         "-d,--device    playback device\n"
         "-r,--rate  stream rate in Hz\n"
         "-c,--channels  count of channels in stream\n"
         "-f,--frequency sine wave frequency in Hz\n"
         "-b,--buffer    ring buffer size in us\n"
         "-p,--period    period size in us\n"
         "-v,--verbose   show the PCM setup parameters\n"
         "\n");
}




static int alsa_init(struct alsa_pcm_state *pcm_state, char *device) {
  snd_pcm_hw_params_t *hw_params;
  int err;

  if ((err = snd_pcm_open(&(pcm_state->pcm_handle), device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    fprintf (stderr, "cannot open audio device %s (%s)\n",  device, snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
    return err;
  }
				 
  if ((err = snd_pcm_hw_params_any(pcm_state->pcm_handle, hw_params)) < 0) {
    fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_access(pcm_state->pcm_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED)) < 0) {
    fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_format(pcm_state->pcm_handle, hw_params, pcm_state->format)) < 0) {
    fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror (err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_rate_near(pcm_state->pcm_handle, hw_params, &pcm_state->sampling_rate, 0)) < 0) {
    fprintf (stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
    return err;
  }
	
  if ((err = snd_pcm_hw_params_set_period_size(pcm_state->pcm_handle, hw_params, pcm_state->period_size, 0)) < 0) {
    fprintf (stderr, "cannot set period size (%s)\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_state->pcm_handle, hw_params, &pcm_state->buffer_size)) < 0) {
    fprintf (stderr, "cannot set buffer size (%s)\n", snd_strerror(err));
    return err;
  }

  if ((err = snd_pcm_hw_params_set_channels(pcm_state->pcm_handle, hw_params, pcm_state->channels)) < 0) {
    fprintf (stderr, "cannot set channel count (%s)\n",
             snd_strerror (err));
    return err;
  }

  if ((err = snd_pcm_hw_params(pcm_state->pcm_handle, hw_params)) < 0) {
    fprintf(stderr, "cannot set parameters (%s)\n",
            snd_strerror(err));
    return err;
  }

  if (verbose) {
    printf("alsa_init: hw_params set\n");
  }
	
  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(pcm_state->pcm_handle)) < 0) {
    fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
            snd_strerror(err));
    return err;
  }

  if (verbose) {
    printf("audio interface prepared\n");
  }

  return 0;
}



static int alsa_pcm_read_spi_write(struct alsa_pcm_state *pcm_state, int spi_handle) {
  ssize_t bytes_transferred = 0;
  int ret;
  snd_pcm_sframes_t commited;
  snd_pcm_sframes_t size;
  snd_pcm_uframes_t offset, frames;
  const snd_pcm_channel_area_t *mmap_area;
  char *samples[pcm_state->channels];
  int steps[pcm_state->channels];
  int channels_times_bytes = pcm_state->channels * (snd_pcm_format_width(pcm_state->format) / 8);
  char sample_to_dac[2];
  uint64_t expirations;
  // what has fewer channels, PCM stream or the DAC?
  int min_channels =
    pcm_state->channels < sizeof(pcm_channel_to_dac_channel_map) / sizeof(pcm_channel_to_dac_channel_map[0]) ?
    pcm_state->channels :
    sizeof(pcm_channel_to_dac_channel_map) / sizeof(pcm_channel_to_dac_channel_map[0]);

  ret = alsa_pcm_ensure_ready(pcm_state);
  if (ret != 0) {
    return ret;
  }

  // on return of the ready func, start our clock to output samples
  if (pcm_state->timer_running == 0) {
    printf("timer armed\n");
    pcm_state->timer_running = 1;
    timerfd_settime(pcm_state->timerfd_sample_clock, 0, &pcm_state->itimerspec_sample_clock, 0);
  }



  size = pcm_state->period_size;
  while (size > 0) {
    frames = size;
    ret = snd_pcm_mmap_begin(pcm_state->pcm_handle, &mmap_area, &offset, &frames);

    //printf("req: size %ld frames %ld (full period %ld)\n", size, frames, pcm_state->period_size);

    if (ret < 0) {
      ret = xrun_recovery(pcm_state, -ret);
      if (ret < 0) {
        fprintf(stderr, "mmap begin avail error: %s\n", snd_strerror(ret));
        return ret;
      }
    }

    bytes_transferred += (frames * channels_times_bytes);

    // calculate the base address for each channelnum and step size
    // the step size ought to be cacheable?  anyway...
    for (int channelnum = 0; channelnum < min_channels; ++channelnum) {
      steps[channelnum] = mmap_area[channelnum].step / 8;
      samples[channelnum] =
        (( mmap_area[channelnum].addr) + (mmap_area[channelnum].first / 8)) +
        offset * steps[channelnum];
    }


    for (int frameno = 0; frameno < frames; ++frameno, size -= frames) {
      // check the clock
      ret = read(pcm_state->timerfd_sample_clock, &expirations, sizeof(expirations));

      if (expirations > 1) {
        // missed an expiration.
        // Reset timer,
        // Advance samples pointers and start next loop iteration.
        // This error handling could use some improvement.
        expirations--;
        frameno += expirations;
        size -= expirations;
        for (int channelnum = 0; channelnum < min_channels; ++channelnum) {
          samples[channelnum] += (expirations * steps[channelnum]);
        }
        timerfd_settime(pcm_state->timerfd_sample_clock, 0, &pcm_state->itimerspec_sample_clock, 0);
        continue;
      }

      for (int channelnum = 0; channelnum < min_channels; ++channelnum) {
        int dac_channel = pcm_channel_to_dac_channel_map[channelnum];
        //printf(" 0x%04hX", *((int16_t*)samples[channelnum]));
        if (*((int16_t*)samples[channelnum]) != pcm_state->previous_sample[channelnum]) {
          sample_to_dac[0] = (dac_channel << 4) |
            (*((int16_t*)samples[channelnum]) >> 11);
          sample_to_dac[1] = *((int16_t*)samples[channelnum]) >> 3;
          spiWrite(spi_handle, sample_to_dac, 2);
          pcm_state->previous_sample[channelnum] = *((int16_t*)samples[channelnum]);
        }
               
        samples[channelnum] += steps[channelnum];
      }
    }


    commited = snd_pcm_mmap_commit(pcm_state->pcm_handle, offset, frames);
    if (commited < 0 || commited != frames) {
      ret = xrun_recovery(pcm_state, commited >= 0 ? EPIPE : -commited);
      if (ret < 0) {
        return ret;
      }
    }

  }

  return bytes_transferred;
}


/* alsa_pcm_ensure_ready
 * check the state to make sure the pcm_handle is good then call snd_pcm_avail_update to make sure
 * available frames is updated.
 * on return the pcm stream should be in a good state for snd_pcm_mmap_begin()
 */
static int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state) {
  int ret, snd_state;
  snd_pcm_sframes_t avail;

  while (1) {
    snd_state = snd_pcm_state(pcm_state->pcm_handle);
    switch (snd_state) {
    case SND_PCM_STATE_XRUN:
    case SND_PCM_STATE_DISCONNECTED:
      ret = xrun_recovery(pcm_state, EPIPE);
      if (ret < 0) {
        return ret;
      }
      break;
    case SND_PCM_STATE_SUSPENDED:
      ret = xrun_recovery(pcm_state, ESTRPIPE);
      if (ret < 0)
        return ret;
      break;
    case SND_PCM_STATE_PREPARED:
    case SND_PCM_STATE_RUNNING:
    default:
      break;
    }

    avail = snd_pcm_avail_update(pcm_state->pcm_handle);
    if (avail < 0) {
      ret = xrun_recovery(pcm_state, -avail);
      if (ret < 0) {
        return ret;
      }
      pcm_state->first_period = 1;
      continue;
    }
    else if (avail < pcm_state->period_size) {
      if (pcm_state->first_period) {
        pcm_state->first_period = 0;
        ret = snd_pcm_start(pcm_state->pcm_handle);
        if (ret < 0) {
          fprintf(stdout, "snd_pcm_start: %s\n", snd_strerror(errno));
          return ret;
        }
      }
      else {
        ret = snd_pcm_wait(pcm_state->pcm_handle, -1);
        if (ret < 0) {
          pcm_state->first_period = 1;
          ret = xrun_recovery(pcm_state, -ret);
          if (ret < 0) {
            return ret;
          }
        }
      }
      continue;
    }
    else {
      break;
    }
  }

  return 0;
}




/** xrun_recovery
 * take the error as a positive int.  Compare versus negatives.
 */
static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err) {

  xrun_recovery_count++;

  if (verbose) {
    printf("stream recovery: error %d\n", err);
  }

  if (err == EPIPE) {    /* under-run */
    err = snd_pcm_prepare(pcm_state->pcm_handle);

    // reset to processing first period
    pcm_state->first_period = 1;
    if (err < 0) {
      printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
    }

    if (pcm_state->timer_running) {
      // reset timer
      timerfd_settime(pcm_state->timerfd_sample_clock, 0, &pcm_state->itimerspec_sample_clock, 0);
    }

    return 0;
  }
  else if (err == ESTRPIPE) {
    while ((err = snd_pcm_resume(pcm_state->pcm_handle)) == -EAGAIN) {
      sleep(1);   /* wait until the suspend flag is released */
    }
    if (err < 0) {
      err = snd_pcm_prepare(pcm_state->pcm_handle);
      if (err < 0) {
        printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
      }
    }

    if (pcm_state->timer_running) {
      // reset timer
      timerfd_settime(pcm_state->timerfd_sample_clock, 0, &pcm_state->itimerspec_sample_clock, 0);
    }

    return 0;
  }
  return err;
}


static void alsa_midi_in_to_i2c(int i2c_handle, struct i2c_gpio_state *gpio_state, snd_rawmidi_t *midi_in, struct midi_state *midi_state) {
  int status = 0;
  uint8_t buffer[128];

  status = snd_rawmidi_read(midi_in, buffer, sizeof(buffer));
  if ((status < 0) && (status != -EBUSY) && (status != -EAGAIN)) {
    printf("Problem reading MIDI input: %s", snd_strerror(status));
    return;
  }

  for (int i = 0; i < status; ++i) {
    if (midi_state->next_byte_is_program) {
      midi_state->next_byte_is_program = 0;
      if ( ! (buffer[i] & 0x80) ) {  // MSB should not be set on a program change
        // this really doesn't't need to be so synchronous-- todo: queue it up and drain
        set_midi_program(i2c_handle, gpio_state, buffer[i]);
      }
      else {
        printf("midi: expected a program change and got 0x%X\n", buffer[i]);
      }
    }
    else {
      // look for a program change status on any channel
      if (buffer[i] & 0xC0) {
        midi_state->next_byte_is_program = 1;
      }
    }
  }
}



// program to switching:
// SYNC_ENABLE_BUTTON_PARAM, { 0, 1 } },
// MIX1_PULSE_BUTTON_PARAM, { 2, 3 } },
// EXT_MOD_SELECT_SWITCH_PARAM, { 4, 5 } },
// MIX1_COMPARATOR_BUTTON_PARAM, { 6, 7 } },
// MIX2_PULSE_BUTTON_PARAM, { 8, 9 } },
// EXT_MOD_PWM_BUTTON_PARAM, { 10, 11 } },
// EXP_FM_BUTTON_PARAM, { 12, 13 } },
// LINEAR_FM_BUTTON_PARAM, { 14, 15 } },
// MIX2_SAW_BUTTON_PARAM, { 16, 17 } },
// { MIX1_SAW_LEVEL_SELECTOR_PARAM, { 18, 19, 20 } }

// set or clear a bit based on the program
struct program_to_gpio {
  int set; // flag for set or clear? 1 set, 0 clear
  int index; // register byte (0 or 1)?
  uint8_t mask;
};
static const struct program_to_gpio program_to_gpio[] = {
  { 0, 0, ~0x40 },  // 0
  { 1, 0,  0x40 },
  { 0, 1, ~0x02 },  // 2
  { 1, 1,  0x02 },
  { 0, 0, ~0x01 },  // 4
  { 1, 0,  0x01 },
  { 0, 1, ~0x01 },  // 6
  { 1, 1,  0x01 },
  { 0, 1, ~0x40 },  // 8
  { 1, 1,  0x40 },
  { 0, 0, ~0x20 },  // 10
  { 1, 0,  0x20 },
  { 0, 0, ~0x80 },  // 12
  { 1, 0,  0x80 },
  { 0, 0, ~0x20 },  // 14
  { 1, 0,  0x20 },
  { 0, 1, ~0x80 },  // 16
  { 1, 1,  0x80 },
  { 0, 1,  0b11110011 },  // 18: Saw
  { 1, 1,  0b00000100 },
  { 1, 1,  0b00001100 },
};
  

static void set_midi_program(int i2c_handle, struct i2c_gpio_state *gpio_state, uint8_t program) {
  int index;
  int err;
  
  if (program < sizeof(program_to_gpio) / sizeof(program_to_gpio[0])) {
    index = program_to_gpio[program].index;
    if (program_to_gpio[program].set) {
      gpio_state->output_state[index] |= program_to_gpio[program].mask;
    }
    else {
      gpio_state->output_state[index] &= program_to_gpio[program].mask;
    }

    err = i2cWriteByteData(i2c_handle, index == 0 ? 0x02 : 0x03, gpio_state->output_state[index]);
    //printf("wrote program 0x%X value 0x%X\n", program, gpio_state->output_state[index]);
    if (err) {
      printf("error writing i2c byte data %d\n", err);
    }

  }
  else {
    printf("program %d out of range to set\n", program);
  }
}
