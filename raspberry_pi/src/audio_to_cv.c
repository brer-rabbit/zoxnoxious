/* 
 * audio_to_cv
 * Compile:
 * gcc -Wall -o audio_to_cv audio_to_cv.c -lasound -lpigpio
 * Run:
 * sudo audio_to_cv hw:CARD=UAC2Gadget,DEV=0
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

// global ref is intended for signal handling only.  don't use globals.  be somewhat civil.
static struct alsa_pcm_state *global_pcm_state;
static int spi_global_handle;
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
  if (global_pcm_state->pcm_handle) {
    snd_pcm_abort(global_pcm_state->pcm_handle);
    snd_pcm_close(global_pcm_state->pcm_handle);
  }

  if (spi_global_handle) {
    spiClose(spi_global_handle);
    gpioTerminate();
  }

  if (global_pcm_state->timerfd_sample_clock) {
    close(global_pcm_state->timerfd_sample_clock);
  }

  if (sig == SIGABRT) {
    /* do not call snd_pcm_close() and abort immediately */
    global_pcm_state->pcm_handle = NULL;
    exit(EXIT_FAILURE);
  }

  printf("xrun_recovery_count: %d\n", xrun_recovery_count);
  signal(sig, SIG_DFL);
  exit(1);
}



int main (int argc, char *argv[]) {
  struct alsa_pcm_state pcm_state = {
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
        .it_interval.tv_nsec = 1000000000 / pcm_state.sampling_rate,
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1000000000 / pcm_state.sampling_rate
      },
    .timer_running = 0,
    .previous_sample = { 0 }
  };
  int spi_handle;
  int err;
  char *device = "plughw:0,0";
  const int spi_rate = 20000000;

  struct option long_option[] = {
    {"help", 0, NULL, 'h'},
    {"device", 1, NULL, 'd'},
    {"rate", 1, NULL, 'r'},
    {"channels", 1, NULL, 'c'},
    {"buffer", 1, NULL, 'b'},
    {"period", 1, NULL, 'p'},
    {"verbose", 1, NULL, 'v'},
    {NULL, 0, NULL, 0},
  };



  while (1) {
    int c;
    if ((c = getopt_long(argc, argv, "hd:r:c:b:p:v", long_option, NULL)) < 0)
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
    case 'v':
      verbose = 1;
      break;
    }
  }


  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGABRT, signal_handler);




  // ALSA INITIALIZE - create handle
  err = alsa_init(&pcm_state, device);
  global_pcm_state = &pcm_state;
  if (err != 0) {
    fprintf(stderr, "alsa init failed\n");
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
  spi_global_handle = spi_handle;

  // END PIGPIO INIT

  printf("alsa init, pigpio init: complete\n");

  //for (int i = 0; i < 400; ++i) {
  while (1) {
    alsa_pcm_read_spi_write(&pcm_state, spi_handle);
    //printf("transferred %d byte\n",
  }


  snd_pcm_close(pcm_state.pcm_handle);
  fprintf(stdout, "audio interface closed\n");

  spiClose(spi_handle);
  gpioTerminate();

  printf("spi closed, gpio terminated\n");

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

    printf("req: size %ld frames %ld (full period %ld)\n", size, frames, pcm_state->period_size);

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
