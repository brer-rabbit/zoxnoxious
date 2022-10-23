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
#include <libconfig.h>
#include <zlog.h>

#include "zoxnoxiousd.h"
#include "zalsa.h"


static int xrun_recovery(struct alsa_pcm_state *pcm_state, int err);
static int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state);


static const snd_pcm_format_t default_snd_pcm_format = SND_PCM_FORMAT_S16_LE;



struct alsa_pcm_state* init_alsa_device(config_t *cfg, int device_num) {
  struct alsa_pcm_state *alsa_state;
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
    INFO("max pcm device id " ZALSA_DEVICES_KEY "[%d]", device_num - 1);
    return NULL;
  }

  // pretty sure we've now got a device name, so initialize it
  alsa_state = (struct alsa_pcm_state*) malloc(sizeof(struct alsa_pcm_state));
  alsa_state->cfg = cfg;
  alsa_state->device_name = strdup(device_name);


  // Params from config file:
  if (config_lookup_int(cfg, ZALSA_BUFFER_SIZE_KEY, &cfg_int_value) == CONFIG_FALSE) {
    INFO("cfg: %s: no buffer size specified, defaulting to %d",
         alsa_state->device_name, ZALSA_DEFAULT_BUFFER_SIZE);
    alsa_state->buffer_size = ZALSA_DEFAULT_BUFFER_SIZE;
  }
  else {
    alsa_state->buffer_size = cfg_int_value;
  }

  if (config_lookup_int(cfg, ZALSA_PERIOD_SIZE_KEY, &cfg_int_value) == CONFIG_FALSE) {
    INFO("cfg: %s:no period size specified, defaulting to %d",
         alsa_state->device_name, ZALSA_DEFAULT_PERIOD_SIZE);
    alsa_state->period_size = ZALSA_DEFAULT_PERIOD_SIZE;
  }
  else {
    alsa_state->period_size = cfg_int_value;

  }

  // Hardcoded defaults:
  alsa_state->format = default_snd_pcm_format;
  alsa_state->first_period = 1;



  // Now open the actual stream
  if ((err = snd_pcm_open(&alsa_state->pcm_handle, alsa_state->device_name, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    ERROR("cannot open audio device %s (%s)",  alsa_state->device_name, snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    ERROR("cannot allocate hardware parameter structure (%s)", snd_strerror(err));
    return NULL;
  }
				 
  if ((err = snd_pcm_hw_params_any(alsa_state->pcm_handle, hw_params)) < 0) {
    ERROR("cannot initialize hardware parameter structure (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_access(alsa_state->pcm_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED)) < 0) {
    ERROR("cannot set access type (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_format(alsa_state->pcm_handle, hw_params, alsa_state->format)) < 0) {
    ERROR("cannot set sample format (%s)", snd_strerror (err));
    return NULL;
  }

  // get min sampling rate and set based on that
  if ((err = snd_pcm_hw_params_get_rate_min(hw_params, &alsa_state->sampling_rate, 0)) < 0) {
    ERROR("cannot get min sampling rate (%s)", snd_strerror(err));
    return NULL;
  }
  if ((err = snd_pcm_hw_params_set_rate_near(alsa_state->pcm_handle, hw_params, &alsa_state->sampling_rate, 0)) < 0) {
    ERROR("cannot set sample rate to %d (%s)", alsa_state->sampling_rate, snd_strerror(err));
    return NULL;
  }
  INFO("set %s sampling rate to %d Hz", alsa_state->device_name, alsa_state->sampling_rate);
	
  if ((err = snd_pcm_hw_params_set_period_size(alsa_state->pcm_handle, hw_params, alsa_state->period_size, 0)) < 0) {
    ERROR("cannot set period size (%s)", snd_strerror(err));
    return NULL;
  }

  if ((err = snd_pcm_hw_params_set_buffer_size_near(alsa_state->pcm_handle, hw_params, &alsa_state->buffer_size)) < 0) {
    ERROR("cannot set buffer size (%s)", snd_strerror(err));
    return NULL;
  }

  // pull in the maximum channels
  if ((err = snd_pcm_hw_params_get_channels_max(hw_params, &alsa_state->channels)) < 0) {
    ERROR("cannot get channel count on %s (%s)", alsa_state->device_name, snd_strerror(err));
  }
  if ((err = snd_pcm_hw_params_set_channels(alsa_state->pcm_handle, hw_params, alsa_state->channels)) < 0) {
    ERROR("cannot set channel count on %s to %d (%s)", alsa_state->device_name,
          alsa_state->channels, snd_strerror(err));
    return NULL;
  }
  INFO("set %s : maximum %d channels set", alsa_state->device_name, alsa_state->channels);


  if ((err = snd_pcm_hw_params(alsa_state->pcm_handle, hw_params)) < 0) {
    ERROR("cannot set parameters (%s)", snd_strerror(err));
    return NULL;
  }

  INFO("alsa_init: %s hardware params set", alsa_state->device_name);
	
  snd_pcm_hw_params_free(hw_params);

  if ((err = snd_pcm_prepare(alsa_state->pcm_handle)) < 0) {
    ERROR("cannot prepare audio interface for use (%s)", snd_strerror(err));
    return NULL;
  }

  INFO("alsa_init: %s prepared", alsa_state->device_name);

  return alsa_state;
}






/* internal static stuff */


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

