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


struct alsa_pcm_state {
  // libconfig handle
  config_t *cfg;

  int device_num;
  char *device_name;
  snd_pcm_t *pcm_handle;
  unsigned int sampling_rate;
  snd_pcm_sframes_t period_size;  // Size to request on read()
  snd_pcm_uframes_t buffer_size;  // size of ALSA buffer (in frames)
  snd_pcm_format_t format;        // audiobuf format
  unsigned int channels;          // number of channels
  int first_period;               // boolean cleared after first frame processed, set after xrun
};



struct alsa_pcm_state* init_alsa_device(config_t *cfg, int device_num) {
  struct alsa_pcm_state *alsa_state;
  snd_pcm_hw_params_t *hw_params;
  int err;
  const char *device_name;

  config_setting_t *devices_setting = config_lookup(cfg, ZALSA_DEVICES);
    
  if (devices_setting == NULL) {
    ERROR("cfg: no device setting found for " ZALSA_DEVICES);
    return NULL;
  }

  device_name = config_setting_get_string_elem(devices_setting, device_num);

  if (device_name == NULL) {
    INFO("max pcm device id " ZALSA_DEVICES "[%d]", device_num - 1);
    return NULL;
  }

  // pretty sure we've now got a device name, so initialize it
  alsa_state = (struct alsa_pcm_state*) malloc(sizeof(struct alsa_pcm_state));
  alsa_state->device_name = strdup(device_name);
  INFO("pcm configured for device %s", alsa_state->device_name);

  return NULL;
}

  /*
  // TODO: get these from libconfig
  alsa_state->buffer_size = buffer_size;
  alsa_state->period_size = period_size;
  alsa_state->format = format;
  alsa_state->channels = channels;
  alsa_state->first_period = 1;


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
*/
