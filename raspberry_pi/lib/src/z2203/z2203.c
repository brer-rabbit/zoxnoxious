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

#define SPI_MODE 1
// Queuee size _must_ be a power of 2 for the bitwise
#define YM2203_QUEUE_SIZE 256
#define YM2203_QUEUE_MASK (YM2203_QUEUE_SIZE - 1)
static_assert((YM2203_QUEUE_SIZE & (YM2203_QUEUE_SIZE - 1)) == 0,
              "YM2203_QUEUE_SIZE must be a power of 2");

static const int spi_channel_cs0 = 0;
static const int spi_channel_cs1 = 1;
static const uint32_t spi_ym2203_write_wait = 3; // delay from address to value write

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

struct ym2203_card {
  struct zhost *zhost;
  int slot;

  struct ym2203_queue queue;

  unsigned int test_counter;
  uint8_t ioa_value; // I/O A register on YM2203
};

static void ym2203_queue_init(struct ym2203_queue *q);
static bool ym2203_queue_pop(struct ym2203_queue *q, struct ym2203_write *write);
static bool ym2203_queue_push(struct ym2203_queue *q, uint8_t address, uint8_t value);
static int ym2203_write_register(struct ym2203_card *card, char address, char value);





void *init_zcard(struct zhost *zhost, int slot) {
    struct ym2203_card *card = calloc(1, sizeof(*card));
    if (!card)
        return NULL;

    card->zhost = zhost;
    card->slot = slot;

    ym2203_queue_init(&card->queue);

    card->test_counter = 0;
    card->ioa_value = 0xff;

    return card;
}


int process_samples(void *zcard_plugin, const int16_t *samples) {
  struct ym2203_card *card = zcard_plugin;

  set_spi_interface(card->zhost, spi_channel_cs0, SPI_MODE, card->slot);

  //  if (!card->hardware_initialized) {
  //    ym2203_initialize(card);
  //    card->hardware_initialized = true;
  //  }

  if (++card->test_counter >= 4000) {
    card->test_counter = 0;

    card->ioa_value ^= 0x02;

    ym2203_write_register(card, 0x0e, card->ioa_value);
  }

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
static int ym2203_write_register(struct ym2203_card *card, char address, char value) {
  int spi_channel_a0, spi_channel_data;
  char a0_low = 0x00, a0_high = 0x01;

  spi_channel_a0 = set_spi_interface(card->zhost, spi_channel_cs0, SPI_MODE, card->slot);
  spi_channel_data = set_spi_interface(card->zhost, spi_channel_cs1, SPI_MODE, card->slot);
  // SPI write A0 low via CS0
  // SPI write address via CVS1
  // delay 3 microseoncds
  // SPI write A0 high via CS0
  // SPI write data via CVS1
  spiWrite(spi_channel_a0, &a0_low, 1);
  spiWrite(spi_channel_data, &address, 1);
   
  gpioDelay(spi_ym2203_write_wait);

  spiWrite(spi_channel_a0, &a0_high, 1);
  spiWrite(spi_channel_data, &value, 1);

  return 0;
}
