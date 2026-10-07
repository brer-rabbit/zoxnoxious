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

  // calculated once processing starts
  int channel_step_size; // step size for each channel in a frame

  // dynamic as we process samples
  snd_pcm_uframes_t offset; // ALSA's starting offset for an mmap region
  snd_pcm_uframes_t frames_provided; // number of contiguous frames from ALSA
  snd_pcm_uframes_t cursor_offset; // app position from offset to frames_provided

  const snd_pcm_channel_area_t *mmap_area;
};


/** alsa_open_device
 *
 * Initialize an alsa pcm device.  Call with a numeric that
 * indexes to a libconfig file.
 */
struct alsa_pcm_state* alsa_open_device(config_t *cfg, int device_num);


/** alsa_pcm_start
 *
 * Wrap calls to snd_pcm_state, snd_pcm_avail_update, snd_pcm_mmap_begin.
 * Return zero for success, non-zero on failure.
 */
int alsa_pcm_start(struct alsa_pcm_state *pcm_state);

/** alsa_advance_cursor
 *
 * Advance the application cursor by the requested number of frames.
 * If the requested position crosses the current mmap region boundary
 * commit the current region and acquire the next contiguous region.
 * If the request is greater than current period the next
 *
 * Return zero for success, negative errno on failure.
 *
 */
int alsa_pcm_advance_cursor(struct alsa_pcm_state *pcm_state, snd_pcm_uframes_t frames_requested);


/** alsa_pcm_close
 *
 * close the pcm stream.
 */
int alsa_pcm_close(struct alsa_pcm_state *pcm);


/** alsa_pcm_cursor_channel
 *
 * based on the cursor, return a pointer to the current sample for this channel.
 * Cursor is not verified as being valid.
 */
static inline const int16_t* alsa_pcm_cursor_sample(const struct alsa_pcm_state *pcm_state, unsigned int channel) {
    return (const int16_t*)(
                            (const char*) pcm_state->mmap_area[channel].addr +
                            pcm_state->mmap_area[channel].first / 8 +
                            (pcm_state->offset + pcm_state->cursor_offset) *
                            pcm_state->channel_step_size);
}

#endif
