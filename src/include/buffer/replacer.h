#ifndef REPLACER_H
#define REPLACER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "buffer/table.h"
#include "buffer/list.h"

typedef int frame_id_t;
#define INVALID_FRAME_ID -1
#define INVALID_FRAME_OFFSET SIZE_MAX

struct replacer {
  size_t capacity; // TODO remove this later - only keeping so tests don't fail
  struct list list;
  struct table table;
  pthread_mutex_t latch;
};

void replacer_init(struct replacer *replacer, int capacity);
void replacer_free(struct replacer *replacer);
void replacer_fix(struct replacer *replacer, frame_id_t frame_id);
void replacer_unfix(struct replacer *replacer, frame_id_t frame_id);
frame_id_t replacer_evict(struct replacer *replacer);
int replacer_size(struct replacer *replacer); //evictable frames

#endif // REPLACER_H
