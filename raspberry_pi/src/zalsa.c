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
#include <errno.h>
#include <libconfig.h>
#include <zlog.h>

#include "zoxnoxiousd.h"
#include "zalsa.h"

#define INVALID_CHANNEL_STEP_SIZE -1
/* wait up to 100ms timeout for the PCM stream */
#define SND_PCM_WAIT_TIMEOUT 100

static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err);

static const snd_pcm_format_t default_snd_pcm_format = SND_PCM_FORMAT_S16_LE;



struct alsa_pcm_state* init_alsa_device(config_t *cfg, int device_num) {
  struct alsa_pcm_state *pcm_state;
  snd_pcm_hw_params_t *hw_params;
  int err;
  int cfg_int_value;
  const char *device_name;


  config_setting_t *devices_setting = config_lookup(cfg, ZALSA_DEVICES_KEY);
    
  if (devices_setting == NULL) {
    ERROR("cfg: no device setting found for " ZALSA_DEVICES_KEY);
    return NULL;
  }

  device_name = config_setting_get_string_elem(devices_setting, device_num);
  

  if (device_name == NULL) {
    // lookup failed, report the previous index as the max the cfg file has
    INFO("Found %d pcm devices", device_num - 1);
    return NULL;
  }

  // pretty sure we've now got a device name, so initialize it
  pcm_state = (struct alsa_pcm_state*) calloc(1, sizeof(struct alsa_pcm_state));
  pcm_state->cfg = cfg;
  pcm_state->device_name = strdup(device_name);
  pcm_state->device_num = device_num;
  pcm_state->channel_step_size = INVALID_CHANNEL_STEP_SIZE;


  // Params from config file:
  if (config_lookup_int(cfg, ZALSA_BUFFER_SIZE_KEY, &cfg_int_value) == CONFIG_FALSE) {
    INFO("cfg: %s: no buffer size specified, defaulting to %d",
         pcm_state->device_name, ZALSA_DEFAULT_BUFFER_SIZE);
    pcm_state->buffer_size = ZALSA_DEFAULT_BUFFER_SIZE;
  }
  else {
    pcm_state->buffer_size = cfg_int_value;
  }

  if (config_lookup_int(cfg, ZALSA_PERIOD_SIZE_KEY, &cfg_int_value) == CONFIG_FALSE) {
    INFO("cfg: %s:no period size specified, defaulting to %d",
         pcm_state->device_name, ZALSA_DEFAULT_PERIOD_SIZE);
    pcm_state->period_size = ZALSA_DEFAULT_PERIOD_SIZE;
  }
  else {
    pcm_state->period_size = cfg_int_value;

  }

  // Hardcoded non-zero defaults:
  pcm_state->format = default_snd_pcm_format;
  pcm_state->first_period = 1;



  // Now open the actual stream
  if ((err = snd_pcm_open(&pcm_state->pcm_handle, pcm_state->device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    ERROR("cannot open audio device %s (%s)",  pcm_state->device_name, snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    ERROR("cannot allocate hardware parameter structure (%s)", snd_strerror(err));
    return NULL;
  }
				 
  if ((err = snd_pcm_hw_params_any(pcm_state->pcm_handle, hw_params)) < 0) {
    ERROR("cannot initialize hardware parameter structure (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_access(pcm_state->pcm_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED)) < 0) {
    ERROR("cannot set access type (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_format(pcm_state->pcm_handle, hw_params, pcm_state->format)) < 0) {
    ERROR("cannot set sample format (%s)", snd_strerror (err));
    return NULL;
  }

  // get min sampling rate and set based on that
  if ((err = snd_pcm_hw_params_get_rate_min(hw_params, &pcm_state->sampling_rate, 0)) < 0) {
    ERROR("cannot get min sampling rate (%s)", snd_strerror(err));
    return NULL;
  }
  if ((err = snd_pcm_hw_params_set_rate_near(pcm_state->pcm_handle, hw_params, &pcm_state->sampling_rate, 0)) < 0) {
    ERROR("cannot set sample rate to %d (%s)", pcm_state->sampling_rate, snd_strerror(err));
    return NULL;
  }
  INFO("set %s sampling rate to %d Hz", pcm_state->device_name, pcm_state->sampling_rate);
	
  if ((err = snd_pcm_hw_params_set_period_size(pcm_state->pcm_handle, hw_params, pcm_state->period_size, 0)) < 0) {
    ERROR("cannot set period size (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_buffer_size_near(pcm_state->pcm_handle, hw_params, &pcm_state->buffer_size)) < 0) {
    ERROR("cannot set buffer size (%s)", snd_strerror(err));
    return NULL;
  }

  // pull in the maximum channels
  if ((err = snd_pcm_hw_params_get_channels_max(hw_params, &pcm_state->channels)) < 0) {
    ERROR("cannot get channel count on %s (%s)", pcm_state->device_name, snd_strerror(err));
  }
  if ((err = snd_pcm_hw_params_set_channels(pcm_state->pcm_handle, hw_params, pcm_state->channels)) < 0) {
    ERROR("cannot set channel count on %s to %d (%s)", pcm_state->device_name,
          pcm_state->channels, snd_strerror(err));
    return NULL;
  }
  INFO("set %s : maximum %d channels set", pcm_state->device_name, pcm_state->channels);

  pcm_state->samples = (const char**)calloc(pcm_state->channels, sizeof(char*));


  if ((err = snd_pcm_hw_params(pcm_state->pcm_handle, hw_params)) < 0) {
    ERROR("cannot set parameters (%s)", snd_strerror(err));
    return NULL;
  }

  INFO("alsa_init: %s (device num %d) hardware params set", pcm_state->device_name, pcm_state->device_num);
	
  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(pcm_state->pcm_handle)) < 0) {
    ERROR("cannot prepare audio interface for use (%s)", snd_strerror(err));
    return NULL;
  }

  INFO("alsa_init: %s prepared", pcm_state->device_name);

  return pcm_state;
}




int alsa_start_stream(struct alsa_pcm_state *pcm_state) {
  int err;
  err = alsa_pcm_ensure_ready(pcm_state);
  if (err == -EAGAIN) {
    return -EAGAIN;
  }
  else if (err) {
    ERROR("zalsa: %s alsa_pcm_ensure_ready error", pcm_state->device_name);
    return 1;
  }

  if ( alsa_mmap_begin_with_step_calc(pcm_state) ) {
    ERROR("zalsa: %s alsa_pcm_mmap_begin error", pcm_state->device_name);
    return 1;
  }

  return 0;
}


int alsa_advance_stream_by_frames(struct alsa_pcm_state *pcm_state, int frames) {
  int retval = 0;
  int err;

  if (pcm_state->frames_remaining > frames) {
    // advance sample pointer by frames --
    // TODO: error handling should use some DSP to smooth this if frames > 1
    for (int i = 0; i < pcm_state->channels; ++i) {
      pcm_state->samples[i] += (frames * pcm_state->channel_step_size);
    }
    pcm_state->frames_remaining -= frames;
  }
  else {
    if ( alsa_mmap_end(pcm_state) ) {
      ERROR("alsa_mmap_end returned non-zero");
      retval = 1;
    }

    err = alsa_pcm_ensure_ready(pcm_state);
    if (err == -EAGAIN) {
      return -EAGAIN;
    }
    else if (err) {
      ERROR("alsa_pcm_ensure_ready returned non-zero");
      retval += 2;
    }

    if ( alsa_mmap_begin(pcm_state) ) {
      ERROR("alsa_mmap_begin returned non-zero");
      retval += 4;
    }
  }

  return retval;
}



/* alsa_pcm_ensure_ready
 * get things ready for a snd_pcm_mmap_begin() call
 */
int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state) {
  int ret, snd_state;
  snd_pcm_sframes_t avail;
  int xrun = 0;

  while (1) {
    snd_state = snd_pcm_state(pcm_state->pcm_handle);
    switch (snd_state) {
    case SND_PCM_STATE_XRUN:
    case SND_PCM_STATE_DISCONNECTED:
      ret = xrun_recovery(pcm_state, EPIPE);
      xrun = 1;
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

    if (xrun) {
      WARN("alsa_pcm_ensure_ready: xrun snd_pcm_avail_update returned %ld", avail);
    }

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
          ERROR("snd_pcm_start: %s\n", snd_strerror(errno));
          return ret;
        }
      }
      else {
        ret = snd_pcm_wait(pcm_state->pcm_handle, SND_PCM_WAIT_TIMEOUT);
        if (ret == 0) {
          return -EAGAIN;
        }
        else if (ret < 0) {
          ret = xrun_recovery(pcm_state, -ret);
          if (ret < 0) {
            return ret;
          }
        }
        if (xrun) {
          WARN("alsa_pcm_ensure_ready: setting first period");
        }
        pcm_state->first_period = 1;
      }
      continue;
    }
    else {
      break;
    }
  }

  return 0;
}




int alsa_mmap_begin_with_step_calc(struct alsa_pcm_state *pcm_state) {
  int ret;
  // some asserts that ought to happen:
  assert(pcm_state != NULL);

  // request period_size of frames
  pcm_state->frames_provided = pcm_state->period_size;
  ret = snd_pcm_mmap_begin(pcm_state->pcm_handle, &pcm_state->mmap_area, &pcm_state->offset, &pcm_state->frames_provided);
  pcm_state->frames_remaining = pcm_state->frames_provided;

  INFO("alsa mmap begin requested %ld frames received %ld frames", pcm_state->period_size, pcm_state->frames_provided);

  if (ret < 0) {
    ret = xrun_recovery(pcm_state, -ret);
    if (ret < 0) {
      ERROR("alsa: mmap begin avail error: %s", snd_strerror(ret));
      return ret;
    }
  }

  // calculate the base address for each channelnum and step size
  for (int channelnum = 0; channelnum < pcm_state->channels; ++channelnum) {
    if (pcm_state->channel_step_size == INVALID_CHANNEL_STEP_SIZE) {
      pcm_state->channel_step_size = pcm_state->mmap_area[channelnum].step / 8;
    }
    else if (pcm_state->channel_step_size != pcm_state->mmap_area[channelnum].step / 8) {
      ERROR("expected consistent channel step size: %d != %d unexpected",
            pcm_state->channel_step_size,
            pcm_state->mmap_area[channelnum].step / 8);
    }

    // locate samples for this channel
    pcm_state->samples[channelnum] =
      pcm_state->mmap_area[channelnum].addr +
      pcm_state->mmap_area[channelnum].first / 8 +
      pcm_state->offset * pcm_state->channel_step_size;
  }


  return 0;
}



int alsa_mmap_begin(struct alsa_pcm_state *pcm_state) {
  int ret;
  // some asserts that ought to happen:
  assert(pcm_state != NULL);

  // request period_size of frames
  pcm_state->frames_provided = pcm_state->period_size;
  ret = snd_pcm_mmap_begin(pcm_state->pcm_handle, &pcm_state->mmap_area, &pcm_state->offset, &pcm_state->frames_provided);

  if (ret < 0) {
    ret = xrun_recovery(pcm_state, -ret);
    if (ret < 0) {
      ERROR("alsa: mmap begin avail error: %s", snd_strerror(ret));
      return ret;
    }
  }
  else {
      pcm_state->frames_remaining = pcm_state->frames_provided;
  }

  // calculate samples address for each channel
  for (int channelnum = 0; channelnum < pcm_state->channels; ++channelnum) {
    pcm_state->samples[channelnum] =
      pcm_state->mmap_area[channelnum].addr +
      pcm_state->mmap_area[channelnum].first / 8 +
      pcm_state->offset * pcm_state->channel_step_size;
  }

  return 0;
}




int alsa_mmap_end(struct alsa_pcm_state *pcm_state) {
  int ret = 0;
  snd_pcm_sframes_t committed;

  committed = snd_pcm_mmap_commit(pcm_state->pcm_handle, pcm_state->offset, pcm_state->frames_provided);
  if (committed < 0 || committed != pcm_state->frames_provided) {
    WARN("alsa_mmap_end commit: xrun_recovery");
    ret = xrun_recovery(pcm_state, committed >= 0 ? EPIPE : -committed);
    if (ret < 0) {
      return ret;
    }
  }
  return ret;
}




/** xrun_recovery
 * take the error as a positive int
 */
static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err) {

  pcm_state->xrun_recovery_count++;

  INFO("stream recovery: error %d", err);

  if (err == EPIPE) {    /* under-run */
    err = snd_pcm_prepare(pcm_state->pcm_handle);

    // reset to processing first period
    pcm_state->first_period = 1;

    if (err < 0) {
      WARN("Can't recovery from underrun, prepare failed: %s", snd_strerror(err));
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
        WARN("Can't recover from suspend, prepare failed: %s", snd_strerror(err));
      }
    }

    return 0;
  }
  return err;
}

