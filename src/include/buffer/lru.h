#ifndef LRU_H
#define LRU_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

struct entry {
  frame_id_t id;
  size_t offset;
};

struct bucket {
  size_t count;
  size_t capacity;
  struct entry *entries;
};

#define BUCKET_COUNT 32768

struct table {
  struct bucket buckets[BUCKET_COUNT];
};

struct lru {
  struct frame *frames;  
  size_t mru;
  size_t lru;
  int capacity; //TODO remove this and allows lru to manage as much as necessary dynamically
  int size; //TODO size_t

  struct table table;
};

void lru_init(struct lru *replacer, int capacity);
void lru_free(struct lru *replacer);
void lru_fix(struct lru *replacer, frame_id_t frame_id);
void lru_unfix(struct lru *replacer, frame_id_t frame_id);
frame_id_t lru_evict(struct lru *replacer);
int lru_size(struct lru *replacer); //evictable frames
int lru_capacity(struct lru *replacer);


frame_id_t lru_debug_mru_at(struct lru *replacer, int idx);

#endif // LRU_H
