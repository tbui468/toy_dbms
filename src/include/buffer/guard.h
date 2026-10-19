#ifndef GUARD_H
#define GUARD_H

#include <stdint.h>
#include <stdbool.h>
#include "storage/page.h"

enum guard_type {
  GT_INVALID,
  GT_READ,
  GT_WRITE
};

struct cache;

struct guard {
  enum guard_type type;
  page_id_t id;
  uint8_t *data;
  struct cache *cache;
};

void guard_init(struct guard *guard, enum guard_type type, page_id_t page_id, struct cache *cache);
void guard_destroy(struct guard *guard);

#endif // GUARD_H
