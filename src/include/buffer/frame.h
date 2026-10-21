#ifndef FRAME_H
#define FRAME_H

#include "storage/page.h"
#include <pthread.h>
#include <stdbool.h>

typedef uint32_t frame_id_t;
#define INVALID_FRAME_ID UINT32_MAX

struct frame_header {
  bool dirty;
  uint32_t pins;
  uint8_t *page_data;
  page_id_t page_id;
  pthread_rwlock_t latch;
};

void frame_header_init(struct frame_header *header, uint8_t *buffer);
void frame_header_destroy(struct frame_header *header);

#endif // FRAME_H
