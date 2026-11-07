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

#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <stdint.h>
#include <stdlib.h>



#define MAX_SYSEX_BYTES 512

enum midi_parse_state {
  MIDI_IDLE,
  MIDI_PROGRAM_DATA,
  MIDI_SYSEX_DATA,
  MIDI_SYSEX_DISCARD
};

// this is all the first nibble stuff- 0xFx stuff is more complex, but it's to be ignored
enum midi_status_byte {
  MIDI_STATUS_NOT_SET = 0x00,
  MIDI_NOTE_OFF = 0x80,
  MIDI_NOTE_ON = 0x90,
  MIDI_KEY_PRESSURE = 0xA0,
  MIDI_CONTROLLER_CHANGE = 0xB0,
  MIDI_PROGRAM_CHANGE = 0xC0,
  MIDI_CHANNEL_PRESSURE = 0xD0,
  MIDI_PITCH_BEND = 0xE0,
  MIDI_SYSEX_START = 0xF0,
  MIDI_TUNE_REQUEST = 0xF6,
  MIDI_SYSEX_END = 0xF7
};


enum midi_message_type {
  MIDI_MSG_SYSEX,
  MIDI_MSG_PROGRAM_CHANGE
};

struct midi_message {
  enum midi_message_type type;

  uint8_t channel;
  uint8_t program;

  const uint8_t *data;
  size_t length;
};

struct midi_parser {
  enum midi_parse_state state;
  uint8_t channel;

  size_t sysex_len;
  uint8_t sysex[MAX_SYSEX_BYTES];

  const uint8_t *input_buffer;
  size_t input_length;
  size_t input_offset;
};

// initialize a parser to set state
void midi_parser_init(struct midi_parser *parser);

// set buffer to parse, expected post-read from snd_rawmidi_read.
// The parser internal state is reset.
void midi_parser_set_input(struct midi_parser *parser, const uint8_t *buffer, size_t length);

// The maintained state of the parser reads from previously set buffer
// of specified length.  Return true if a complete MIDI message is
// found in the buffer (which may be state from a previous call).
// False when all complete MIDI messages in the buffer are exhausted.
int midi_parser_next(struct midi_parser *parser, struct midi_message *message);

#endif
