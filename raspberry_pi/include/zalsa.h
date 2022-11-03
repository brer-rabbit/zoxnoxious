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

#include <libconfig.h>

#ifndef ZALSA_H
#define ZALSA_H

// config lookup keys
#define ZALSA_DEVICES_KEY "zalsa.devices"
#define ZALSA_PERIOD_SIZE_KEY "zalsa.period_size"
#define ZALSA_BUFFER_SIZE_KEY "zalsa.buffer_size"

#define ZALSA_DEFAULT_PERIOD_SIZE 32
#define ZALSA_DEFAULT_BUFFER_SIZE 64
#define ABSOLUTE_MAX_CHANNELS 32  // we'll never have greater than this number of channels

struct alsa_pcm_state {
  // libconfig handle
  config_t *cfg;
  // set once type stuff
  int device_num;
  char *device_name;
  snd_pcm_t *pcm_handle;
  unsigned int sampling_rate;
  snd_pcm_sframes_t period_size;  // Size to request on read(), frames to request
  snd_pcm_uframes_t buffer_size;  // size of ALSA buffer (in frames)
  snd_pcm_format_t format;        // audiobuf format
  unsigned int channels;          // number of channels

  int first_period;               // boolean cleared after first frame processed, set after xrun

  // calculated once processing starts
  int channel_step_size; // step size for each channel in a frame

  // dynamic as we process samples
  snd_pcm_uframes_t frames_provided;
  snd_pcm_uframes_t frames_remaining;
  snd_pcm_uframes_t offset;
  const snd_pcm_channel_area_t *mmap_area;
  const char **samples; // pointer per-channel to sample data: allocated during 

  // stats
  int xrun_recovery_count;
  int skiped_samples;
};


/** init_alsa_device
 *
 * Initialize an alsa pcm device.  Call with a numeric that
 * indexes to a libconfig file.  Huh, that's kinda a wonky
 * interface.
 */
struct alsa_pcm_state* init_alsa_device(config_t *cfg, int device_num);


/** alsa_start_stream
 *
 * Wrap calls to snd_pcm_state, snd_pcm_avail_update, snd_pcm_mmap_begin.
 * Return zero for success, non-zero on failure.
 */
int alsa_start_stream(struct alsa_pcm_state *pcm_state);

/** alsa_advance_stream_by_frames
 *
 * advance the samples pointers by frames...hopefully one.
 * If the request is greater than current period the next
 * period is requested.  There's likely a bug in there.
 * This wraps calls to snd_pcm_mmap_commit, snd_pcm_state, snd_pcm_avail_update, snd_pcm_mmap_begin.
 * Return zero for success, non-zero on failure.
 */
int alsa_advance_stream_by_frames(struct alsa_pcm_state *pcm_state, int frames);



/** alsa_pcm_ensure_ready
 * check the state to make sure the pcm_handle is good then call snd_pcm_avail_update to make sure
 * available frames is updated.
 * This call is required prior to calling snd_pcm_mmap_begin(), and
 * on return the pcm stream should be in a good state.
 */
int alsa_pcm_ensure_ready(struct alsa_pcm_state *pcm_state);


/** alsa_mmap_begin_with_step_calc alsa_mmap_begin
 *
 * call snd_pcm_mmap_begin.  Pre: alsa_pcm_ensure_ready called just previous.
 * Handle any xrun recovery required.
 * alsa_mmap_begin_with_step_calc should be called the first time through; after
 * that alsa_mmap_begin is preferred.
 */
int alsa_mmap_begin_with_step_calc(struct alsa_pcm_state *pcm_state);
int alsa_mmap_begin(struct alsa_pcm_state *pcm_state);


/** alsa_mmap_end
 *
 * it's called mmap_commit in the alsa API, call the function mmap_end
 * here.  Handle any xrun error.  Return zero for success, non-zero
 * for error.
 */
int alsa_mmap_end(struct alsa_pcm_state *pcm_state);


/** alsa_drop_frames
 *
 * advance internal pointer, dropping the specified number of frames.
 */
void alsa_drop_frames(struct alsa_pcm_state *pcm_state, unsigned long frames);





#endif
