#ifndef GUARD_H
#define GUARD_H

#include <stdint.h>
#include <stdbool.h>
#include "storage/page.h"
#include "buffer/frame.h"

enum guard_type {
  GT_INVALID,
  GT_READ,
  GT_WRITE
};

struct cache;

struct guard {
  enum guard_type type;
  struct cache *cache;
  struct frame_header *frame_header;
};

void guard_init(struct guard *guard, enum guard_type type, struct cache *cache, page_id_t page_id);
void guard_destroy(struct guard *guard);
const uint8_t* guard_raw_immut(struct guard *guard);
uint8_t* guard_raw_mut(struct guard *guard);

#endif // GUARD_H
