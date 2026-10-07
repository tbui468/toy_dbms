#ifndef REPLACER_H
#define REPLACER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "buffer/table.h"

typedef int frame_id_t;
#define INVALID_FRAME_ID -1
#define INVALID_FRAME_OFFSET SIZE_MAX

struct frame;
struct frame {
  size_t next;
  size_t prev;
  frame_id_t id;
};

void frame_init(struct frame *frame, frame_id_t id);

struct replacer {
  struct frame *frames;  
  size_t mru;
  size_t lru;
  int capacity; //TODO remove this and allows replacer to manage as much as necessary dynamically
  int size; //TODO size_t

  struct table table;
};

void replacer_init(struct replacer *replacer, int capacity);
void replacer_free(struct replacer *replacer);
void replacer_fix(struct replacer *replacer, frame_id_t frame_id);
void replacer_unfix(struct replacer *replacer, frame_id_t frame_id);
frame_id_t replacer_evict(struct replacer *replacer);
int replacer_size(struct replacer *replacer); //evictable frames
int replacer_capacity(struct replacer *replacer);


frame_id_t replacer_debug_mru_at(struct replacer *replacer, int idx);

#endif // REPLACER_H
