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

static int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state);
static int alsa_mmap_begin_with_step_calc(struct alsa_pcm_state *pcm_state);
static int alsa_mmap_begin(struct alsa_pcm_state *pcm_state);
static int alsa_mmap_end(struct alsa_pcm_state *pcm_state);
static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err);
static void alsa_advance_mmap_cursor(struct alsa_pcm_state *pcm_state, snd_pcm_uframes_t frames);

static const snd_pcm_format_t default_snd_pcm_format = SND_PCM_FORMAT_S16_LE;



struct alsa_pcm_state* alsa_open_device(config_t *cfg, int device_num) {
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




int alsa_pcm_start(struct alsa_pcm_state *pcm_state) {
  int err;
  err = alsa_pcm_ensure_ready(pcm_state);
  if (err) {
    return err;
  }

  err = alsa_mmap_begin_with_step_calc(pcm_state);
  if (err) {
    ERROR("zalsa: %s alsa_pcm_mmap_begin error", pcm_state->device_name);
    return err;
  }

  return 0;
}

/*
 * requested < remaining
 * --> destination is inside current mmap
 * --> advance
 *
 * requested >= remaining
 * --> destination is outside current mmap
 * --> acquire next mmap, calculate a residual advance
 *
 * residual < remaining
 * --> destination is inside next mmap
 * --> advance
 *
 * residual >= remaining
 * --> destination is outside next mmap
 * --> abandon residual
 */
int alsa_advance_cursor(struct alsa_pcm_state *pcm_state, snd_pcm_uframes_t frames_requested) {
  int err;
  snd_pcm_uframes_t residual_advance;


  if (pcm_state->frames_provided == 0) {
    /* No active mmap region. Establish a new cursor at index 0.  The
     * requested advancement cannot be applied to the old cursor
     * because it no longer exists.
     */
    err = alsa_pcm_ensure_ready(pcm_state);
    if (err) {
      return err;
    }

    return alsa_mmap_begin(pcm_state);
  }

  if (frames_requested < pcm_state->frames_remaining) {
    alsa_advance_mmap_cursor(pcm_state, frames_requested);
    return 0;
  }

  residual_advance = frames_requested - pcm_state->frames_remaining;

  // advancing by frames_requested is more than what is available, get a new mmap
  if (pcm_state->frames_provided > 0) {
    err = alsa_mmap_end(pcm_state);
    if (err) {
      return err;
    }
  }

  err = alsa_pcm_ensure_ready(pcm_state);
  if (err) {
    return err;
  }

  err = alsa_mmap_begin(pcm_state);
  if (err) {
    return err;
  }

  if (residual_advance > 0 &&
      residual_advance < pcm_state->frames_remaining) {
    alsa_advance_mmap_cursor(pcm_state, residual_advance);
  }
  // else start cursor at zero on new mmap region

  return 0;
}




int alsa_pcm_close(struct alsa_pcm_state *pcm) {
  if (pcm && pcm->handle) {
    snd_pcm_hw_params_free(pcm->handle);

    snd_pcm_abort(pcm->pcm_handle);
    return snd_pcm_close(pcm->pcm_handle);
  }
  return -EBADF;
}




/* alsa_pcm_ensure_ready
 * get things ready for a snd_pcm_mmap_begin() call
 * This function should own set/clear of first_period
 * Returns:
 *   0         PCM is ready; snd_pcm_mmap_begin() should succeed.
 *   -EAGAIN   PCM is not ready yet; caller should try again later.
 *   < 0       unrecovered ALSA error.
 */
static int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state) {
  int ret;
  snd_pcm_state_t snd_state;
  snd_pcm_sframes_t avail;

  while (1) {
    snd_state = snd_pcm_state(pcm_state->pcm_handle);
    switch (snd_state) {
    case SND_PCM_STATE_XRUN:
      ret = xrun_recovery(pcm_state, -EPIPE);
      if (ret < 0) {
        return ret;
      }
      continue;
    case SND_PCM_STATE_SUSPENDED:
      ret = xrun_recovery(pcm_state, -ESTRPIPE);
      if (ret < 0) { // -EAGAIN or otherwise
        return ret;
      }
      continue;
    case SND_PCM_STATE_DISCONNECTED:
      return -ENODEV;
    case SND_PCM_STATE_PREPARED:
    case SND_PCM_STATE_RUNNING:
      break;
    default:
      ERROR("Unexpected PCM state: %s", snd_pcm_state_name(snd_state));
      return -EBADFD;
    }

    avail = snd_pcm_avail_update(pcm_state->pcm_handle);

    if (avail < 0) {
      // handle recoverable errors
      ret = xrun_recovery(pcm_state, avail);
      if (ret < 0) {
        return ret;
      }
      continue;
    }

    if (snd_state == SND_PCM_STATE_PREPARED) {
      ret = snd_pcm_start(pcm_state->pcm_handle);
      if (ret < 0) {
        ERROR("snd_pcm_start: %s", snd_strerror(ret));
        return ret;
      }
      // we are RUNNING
      continue;
    }

    // RUNNING
    // TODO: this check may not be necessary.  Process frame by frame, right?
    if (avail < pcm_state->period_size) {
      return -EAGAIN;
    }

    return 0;
  }
}




static int alsa_mmap_begin_with_step_calc(struct alsa_pcm_state *pcm_state) {
  int ret;

  assert(pcm_state != NULL);

  // request period_size of frames
  pcm_state->frames_provided = pcm_state->period_size;
  ret = snd_pcm_mmap_begin(pcm_state->pcm_handle, &pcm_state->mmap_area, &pcm_state->offset, &pcm_state->frames_provided);

  if (ret < 0) {
    ret = xrun_recovery(pcm_state, ret);
    if (ret < 0) {
      ERROR("alsa: mmap begin avail error: %s", snd_strerror(ret));
      return ret;
    }
    return -EAGAIN;
  }

  pcm_state->frames_remaining = pcm_state->frames_provided;
  INFO("alsa mmap begin requested %ld frames received %ld frames", pcm_state->period_size, pcm_state->frames_provided);

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



static int alsa_mmap_begin(struct alsa_pcm_state *pcm_state) {
  int ret;
  // some asserts that ought to happen:
  assert(pcm_state != NULL);

  // request period_size of frames
  pcm_state->frames_provided = pcm_state->period_size;
  ret = snd_pcm_mmap_begin(pcm_state->pcm_handle, &pcm_state->mmap_area, &pcm_state->offset, &pcm_state->frames_provided);

  if (ret < 0) {
    int recovery_ret = xrun_recovery(pcm_state, ret);
    if (recovery_ret < 0) {
      ERROR("alsa: mmap begin avail error: %s", snd_strerror(recovery_ret));
      return recovery_ret;
    }
    return -EAGAIN;
  }

  pcm_state->frames_remaining = pcm_state->frames_provided;

  // calculate samples address for each channel
  for (int channelnum = 0; channelnum < pcm_state->channels; ++channelnum) {
    pcm_state->samples[channelnum] =
      pcm_state->mmap_area[channelnum].addr +
      pcm_state->mmap_area[channelnum].first / 8 +
      pcm_state->offset * pcm_state->channel_step_size;
  }

  return 0;
}




static int alsa_mmap_end(struct alsa_pcm_state *pcm_state) {
  int ret = 0;
  snd_pcm_sframes_t committed;

  // note that snd_pcm_mmap_commit should force a hardware pointer sync
  committed = snd_pcm_mmap_commit(pcm_state->pcm_handle, pcm_state->offset, pcm_state->frames_provided);
  if (committed < 0 || committed != pcm_state->frames_provided) {
    WARN("alsa_mmap_end commit: xrun_recovery");
    ret = xrun_recovery(pcm_state, committed >= 0 ? -EPIPE : committed);
  }

  pcm_state->frames_provided = 0;
  pcm_state->frames_remaining = 0;
  return ret;
}




/** xrun_recovery
 * Attempt recovery from an ALSA PCM error.
 * Return val:
 *   0   recovery succeeded
 *   <0  recovery failed, or the error is not recoverable here
 */
static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err) {
  int ret;
  //DEBUG("stream recovery: error (%d) %s", err, snd_strerror(err));

  if (err == -EPIPE) { /* under-run */
    ret = snd_pcm_prepare(pcm_state->pcm_handle);
    if (ret < 0) {
      //WARN("Can't recover from underrun, prepare failed: %s", snd_strerror(ret));
      return ret;
    }

    return 0;
  }
  else if (err == -ESTRPIPE) { /* suspended */
    ret = snd_pcm_resume(pcm_state->pcm_handle);

    if (ret == -EAGAIN) {
      // resume isn't possible yet-- don't block, let the caller deal with it
      return -EAGAIN;
    }
    else if (ret < 0) {
      ret = snd_pcm_prepare(pcm_state->pcm_handle);
      if (ret < 0) {
        WARN("Can't recover from suspend, prepare failed: %s", snd_strerror(ret));
        return ret;
      }
    }

    return 0;
  }

  // not an error this function can recover from
  return err;
}


// advance the internal samples cursor for each channel by the requested number of frames.
// No error checking is done; assume the frames argument is within the mmap region.
static void alsa_advance_mmap_cursor(struct alsa_pcm_state *pcm_state, snd_pcm_uframes_t frames) {
  for (snd_pcm_uframes_t i = 0; i < pcm_state->channels; ++i) {
    pcm_state->samples[i] += frames * pcm_state->channel_step_size;
  }

  pcm_state->frames_remaining -= frames;
}
