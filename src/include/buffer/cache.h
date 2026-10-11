#ifndef BPM_H
#define BPM_H

#include "storage/page.h"

#include <stdlib.h>
#include "buffer/lru.h"

struct bpm {
  size_t size;
  size_t next_page_id;
  struct lru replacer;
};


void bpm_init(struct bpm *bpm, int size);
int bpm_size(const struct bpm *bpm);
page_id_t bpm_new_page(struct bpm *bpm);
// bpm_flush_page
// bpm_pin_count

enum guard_type {
  GT_READ,
  GT_WRITE
};

// TODO rename to struct page, and move to page.h
struct guard {
  enum guard_type type;
  struct bpm *bpm;
  bool dirty;
};

void rguard_init(struct guard *guard, struct bpm *bpm, page_id_t page_id);
void rguard_free(struct guard *guard);

//r/wlatch_init
//  should increment pin
//  need to latch bpm before doing anything to ensure thread-safe
//r/wlatch_free
//  should decrement pin and unfix if necessary
//flush
//increment_pin
//decrement_pin

#endif // BPM_H
