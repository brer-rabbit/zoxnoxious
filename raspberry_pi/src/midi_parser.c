/* Copyright 2026 Kyle Farrell
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

#include "midi_parser.h"

void midi_parser_init(struct midi_parser *parser) {
    *parser = (struct midi_parser) {
        .state = MIDI_IDLE
    };
}



void midi_parser_set_input(struct midi_parser *parser,
                           const uint8_t *buffer,
                           size_t length) {
  parser->input_buffer = buffer;
  parser->input_length = length;
  parser->input_offset = 0;
}



int midi_parser_next(struct midi_parser *parser, struct midi_message *message) {

  while (parser->input_offset < parser->input_length) {
    uint8_t b = parser->input_buffer[parser->input_offset++];

    // real time messages may appear anytime; ignore
    if (b >= 0xF8) {
      continue;
    }

    // Trust the status bytes: begin new messages
    if (b == MIDI_SYSEX_START) {
      parser->state = MIDI_SYSEX_DATA;
      parser->sysex_len = 0;
      parser->sysex[parser->sysex_len++] = b;
      continue;
    }

    // complete a SysEx message then go idle
    if (b == MIDI_SYSEX_END) {
      if (parser->state == MIDI_SYSEX_DATA) {
        parser->sysex[parser->sysex_len++] = b;

        *message = (struct midi_message) {
          .type = MIDI_MSG_SYSEX,
          .data = parser->sysex,
          .length = parser->sysex_len
        };

        parser->state = MIDI_IDLE;
        return 1;
      }

      parser->state = MIDI_IDLE;
      continue;
    }

    // Any other status byte interrupts the current message
    if (b & 0x80) {
      parser->state = MIDI_IDLE;

      if ((b & 0xF0) == MIDI_PROGRAM_CHANGE) {
        parser->state = MIDI_PROGRAM_DATA;
        parser->channel = b & 0x0F;
      }
      else if (b == MIDI_TUNE_REQUEST) {
        *message = (struct midi_message) {
          .type = MIDI_MSG_TUNE_REQUEST
        };
        return 1;
      }

      continue;
    }

    // data byte: interpret according to parser state.
    switch (parser->state) {
    case MIDI_IDLE:
      // in the idle state-- ignore
      break;

    case MIDI_PROGRAM_DATA:
      *message = (struct midi_message) {
        .type = MIDI_MSG_PROGRAM_CHANGE,
        .channel = parser->channel,
        .program = b
      };
      parser->state = MIDI_IDLE;
      return 1;

    case MIDI_SYSEX_DATA:
      // Reserve one byte for the F7 terminator.
      if (parser->sysex_len < sizeof(parser->sysex) - 1) {
        parser->sysex[ parser->sysex_len++ ] = b;
      } else {
        parser->state = MIDI_SYSEX_DISCARD;
      }
      break;

    case MIDI_SYSEX_DISCARD:
      // Ignore data until F7 or a new status.
      break;
    }
  }

  // input exhausted, but preserve message state.
  return 0;
}
