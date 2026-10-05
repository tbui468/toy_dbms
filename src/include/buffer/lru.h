#ifndef LRU_H
#define LRU_H

#include <stdint.h>
#include <stdbool.h>

typedef int page_id_t;
typedef int frame_id_t;
#define INVALID_PAGE_ID -1
#define INVALID_FRAME_ID -1
#define PAGE_SIZE 8192

struct frame;
struct frame {
  struct frame *next;
  struct frame *prev;
  bool evictable;
};

void frame_init(struct frame *frame, frame_id_t id);

struct lru {
  struct frame *frames;  
  struct frame *mru;
  struct frame *lru;
  int capacity;
  int size;
};

void lru_init(struct lru *replacer, int capacity);
void lru_free(struct lru *replacer);
void lru_set_evictable(struct lru *replacer, frame_id_t frame_id, bool pinned);
frame_id_t lru_evict(struct lru *replacer);
void lru_record_access(struct lru *replacer, frame_id_t);
void lru_remove(struct lru *replacer, frame_id_t frame_id);
int lru_size(struct lru *replacer); //evictable frames
int lru_capacity(struct lru *replacer);


frame_id_t lru_debug_mru_at(struct lru *replacer, int idx);

#endif // LRU_H
