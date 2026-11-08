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

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "zcard_plugin.h"

#define SPI_MODE 0
// Queuee size _must_ be a power of 2 for the bitwise operations
#define YM2203_QUEUE_SIZE 256
#define YM2203_QUEUE_MASK (YM2203_QUEUE_SIZE - 1)
static_assert((YM2203_QUEUE_SIZE & (YM2203_QUEUE_SIZE - 1)) == 0,
              "YM2203_QUEUE_SIZE must be a power of 2");

static const int spi_channel_cs0 = 0;
static const int spi_channel_cs1 = 1;
static const uint32_t spi_ym2203_write_address_wait = 3; // delay after address writes
static const uint32_t spi_ym2203_write_data_wait = 25; // delay after data writes

// ym2203 register write modeled as address & value
struct ym2203_write {
  char address;
  char value;
};

struct ym2203_queue {
  struct ym2203_write entries[YM2203_QUEUE_SIZE];
  _Atomic unsigned int head;
  _Atomic unsigned int tail;
};

struct ym2203_zcard {
  struct zhost *zhost;
  int slot;

  struct ym2203_queue queue;

  unsigned int test_counter;
  uint8_t ioa_value; // I/O A register on YM2203
  uint8_t key_on_off;
};

static void ym2203_queue_init(struct ym2203_queue *q);
static bool ym2203_queue_pop(struct ym2203_queue *q, struct ym2203_write *write);
static bool ym2203_queue_push(struct ym2203_queue *q, uint8_t address, uint8_t value);
static int ym2203_write_register(struct ym2203_zcard *card, char address, char value);





void *init_zcard(struct zhost *zhost, int slot) {
    struct ym2203_zcard *zcard = calloc(1, sizeof(struct ym2203_zcard));
    if (!zcard) {
      return NULL;
    }

    zcard->zhost = zhost;
    zcard->slot = slot;

    ym2203_queue_init(&zcard->queue);

    zcard->test_counter = 0;
    zcard->ioa_value = 0xff;
    zcard->key_on_off = 0x10;

    ym2203_write_register(zcard, 0x00, 0xff);
    ym2203_write_register(zcard, 0x01, 0x00);
    // purportedly this sets port B & A to output
    ym2203_write_register(zcard, 0x07, 0xfe); // was: c0
    gpioDelay(spi_ym2203_write_data_wait);

    /* Channel A fixed volume = maximum */
    ym2203_write_register(zcard, 0x08, 0x0f);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: DT=0, MULTI=1 */
    ym2203_write_register(zcard, 0x30, 0x01);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: TL=0, maximum amplitude */
    ym2203_write_register(zcard, 0x40, 0x00);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: KS=0, AR=31 */
    ym2203_write_register(zcard, 0x50, 0x1f);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: DR=0 */
    ym2203_write_register(zcard, 0x60, 0x00);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: SR=0 */
    ym2203_write_register(zcard, 0x70, 0x00);
    gpioDelay(spi_ym2203_write_data_wait);

    /* OP1: SL=0, RR=15 */
    ym2203_write_register(zcard, 0x80, 0x0f);
    gpioDelay(spi_ym2203_write_data_wait);

    /* Channel 1 algorithm 7 */
    ym2203_write_register(zcard, 0xb0, 0x07);
    gpioDelay(spi_ym2203_write_data_wait);

    /* Frequency */
    ym2203_write_register(zcard, 0xa4, 0x17);
    gpioDelay(spi_ym2203_write_data_wait);

    ym2203_write_register(zcard, 0xa0, 0x2f);
    gpioDelay(spi_ym2203_write_data_wait);

    return zcard;
}


void free_zcard(void *zcard_plugin) {
  struct ym2203_zcard *z2203 = (struct ym2203_zcard*)zcard_plugin;

  if (z2203) {
    free(z2203);
  }
}


char* get_plugin_name() {
  return "Zoxnoxious 2203";
}


struct zcard_properties* get_zcard_properties() {
  struct zcard_properties *props = (struct zcard_properties*) malloc(sizeof(struct zcard_properties));
  props->num_channels = 0;
  props->spi_mode = SPI_MODE;
  return props;
}


int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct ym2203_zcard *card = zcard_plugin;

  if (++card->test_counter >= 4000) {
    card->test_counter = 0;

    card->key_on_off = card->key_on_off == 0x00 ? 0xf0 : 0x00;
    ym2203_write_register(card, 0x28, card->key_on_off);
  }

  // return the number of SPI transactions
  return 4;
}


int process_midi(void *zcard_plugin, const uint8_t *midi_message, size_t size) {
  return 0;
}

int process_midi_program_change(void *zcard_plugin, uint8_t program_number) {
  return 0;
}



int tunereq_save_state(void *zcard_plugin) {
  return 0;
}

tune_status_t tunereq_set_point(void *zcard_plugin) {
  return TUNE_COMPLETE_SUCCESS;
}

tune_status_t tunereq_measurement(void *zcard_plugin, struct tuning_measurement *tuning_measurement) {
  return TUNE_COMPLETE_SUCCESS;
}

int tunereq_restore_state(void *zcard_plugin) {
  return 0;
}

// ----------------
// static functions


static void ym2203_queue_init(struct ym2203_queue *q) {
  atomic_init(&q->head, 0);
  atomic_init(&q->tail, 0);
}


static bool ym2203_queue_push(struct ym2203_queue *q, uint8_t address, uint8_t value) {
  unsigned int head = atomic_load_explicit(&q->head, memory_order_relaxed);
  unsigned int next = (head + 1) & YM2203_QUEUE_MASK;
  unsigned int tail = atomic_load_explicit(&q->tail, memory_order_acquire);

  if (next == tail) {
    return false; /* queue full */
  }

  q->entries[head].address = address;
  q->entries[head].value   = value;

  atomic_store_explicit(&q->head, next, memory_order_release);
  return true;
}

static bool ym2203_queue_pop(struct ym2203_queue *q, struct ym2203_write *write) {
  unsigned int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
  unsigned int head = atomic_load_explicit(&q->head, memory_order_acquire);

  if (tail == head) {
    return false; /* queue empty */
  }

  *write = q->entries[tail];

  unsigned int next = (tail + 1) & YM2203_QUEUE_MASK;

  atomic_store_explicit(&q->tail, next, memory_order_release);
  return true;
}


// write the 8-bit value to the register at address
static int ym2203_write_register(struct ym2203_zcard *card, char address, char value) {
  int spi_channel_a0, spi_channel_data;
  char a0_low = 0x00, a0_high = 0x80;

  a0_high = a0_high | ((value & 0x02) >> 1); // U7 QA LED mirrors IOA1

  spi_channel_a0 = set_spi_interface(card->zhost, spi_channel_cs0, SPI_MODE, card->slot);
  spi_channel_data = set_spi_interface(card->zhost, spi_channel_cs1, SPI_MODE, card->slot);
  // SPI write A0 low via CS0
  // SPI write address via CVS1
  // delay 3 microseoncds
  // SPI write A0 high via CS0
  // SPI write data via CVS1
  spiWrite(spi_channel_a0, &a0_low, 1);
  spiWrite(spi_channel_data, &address, 1);
   
  gpioDelay(spi_ym2203_write_address_wait);

  spiWrite(spi_channel_a0, &a0_high, 1);
  spiWrite(spi_channel_data, &value, 1);

  return 0;
}
