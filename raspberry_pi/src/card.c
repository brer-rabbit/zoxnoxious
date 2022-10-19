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


#include <pigpio.h>
#include <stdlib.h>
#include <zlog.h>

#include "zoxnoxiousd.h"



int discover_cards(int i2c_address, struct plugin_card **plugin_cards, int *num_cards_found) {
  int i2c_handle;
  int byte_read;
  int card_id;

  /* this should probe 8 sequential I2C addresses in the range
   * i2c_address to find what is present
   */
  *num_cards_found = 3;
  *plugin_cards = (struct plugin_card*) calloc(3, sizeof(struct plugin_card));

  *plugin_cards[0] = (struct plugin_card)
    {
      .slot = 0,
      .card_id = 0x02,
      .plugin_name = "VCO3340",
      .process_samples_f = NULL,
      .process_midi_f = NULL,
      .process_midi_program_change_f = NULL,
      .free_zcard_f = NULL
    };

  for (int i = 0; i < 8; ++i) {
    i2c_handle = i2cOpen(1, i2c_address + i, 0);
    if (i2c_handle >= 0) {
      card_id = i2cReadByteData(i2c_handle, 0);
      if (card_id == PI_BAD_HANDLE ||
          card_id == PI_BAD_PARAM) {
        INFO("I2C read bad handle for slot %d", i);
      }
      else if (card_id == PI_I2C_READ_FAILED) {
        INFO("no card present slot %d", i);
      }
      else {
        INFO("Found card in slot %d with id 0x%x", i, card_id);
        // TODO: action
      }

    }
    else {
      INFO("failed to open handle for address %d\n", i2c_address + i);
    }
  }


  return 0;
}
