#include "buffer/lru.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void frame_init(struct frame *frame, frame_id_t id) {
  frame->next = NULL;
  frame->prev = NULL;
  frame->evictable = false;
}

void lru_init(struct lru *replacer, int capacity) {
  replacer->capacity = capacity;
  replacer->frames = malloc(sizeof(struct frame) * replacer->capacity);
  for (int i = 0; i < replacer->capacity; i++) {
    frame_init(replacer->frames + i, i);
  }
  replacer->size = 0;
  replacer->lru = NULL;
  replacer->mru = NULL;
}

void lru_free(struct lru *replacer) {
  free(replacer->frames);
}

int lru_size(struct lru *replacer) {
  return replacer->size;
}

int lru_capacity(struct lru *replacer) {
  return replacer->capacity;
}

void lru_set_evictable(struct lru *replacer, frame_id_t frame_id, bool evictable) {
  struct frame *frame = &replacer->frames[frame_id];
  replacer->size += evictable - frame->evictable;
  frame->evictable = evictable;
}

static void lru_do_remove(struct lru *replacer, frame_id_t frame_id, bool decrement_size) {
  struct frame *frame = &replacer->frames[frame_id];
  struct frame *prev = frame->prev;
  struct frame *next = frame->next;

  if (prev) {
    prev->next = frame->next;
  } else {
    replacer->mru = frame->next;  
  }

  if (next) {
    next->prev = frame->prev;
  } else {
    replacer->lru = prev; 
  }

  if (decrement_size && frame->evictable) {
    replacer->size--;
  }
}

void lru_remove(struct lru *replacer, frame_id_t frame_id) {
  lru_do_remove(replacer, frame_id, true);
}


frame_id_t lru_evict(struct lru *replacer) {
  struct frame *frame = replacer->lru;
  while (frame && !frame->evictable) {
    frame = frame->prev;
  }

  if (frame) {
    frame_id_t frame_id = frame - replacer->frames;
    lru_remove(replacer, frame_id);
    frame_init(frame, frame_id);
    return frame_id;
  }

  return INVALID_FRAME_ID;
}

static void lru_insert_into_mru(struct lru *replacer, frame_id_t frame_id) {
  struct frame *frame = &replacer->frames[frame_id];
  frame->prev = NULL;

  struct frame *prev_mru = replacer->mru;
  frame->next = replacer->mru;

  if (prev_mru) {
    prev_mru->prev = frame;
  }

  if (!replacer->lru) {
    replacer->lru = frame;
  }

  replacer->mru = frame;
}

void lru_record_access(struct lru *replacer, frame_id_t frame_id) {
  assert(frame_id < replacer->capacity);
  struct frame *frame = &replacer->frames[frame_id];

  // TODO change to if (lru_frame_unused(replacer, frame_id)) {
  //if (frame->page_id == INVALID_PAGE_ID) {
  if (!frame->next && !frame->prev && replacer->mru != frame) {
    lru_insert_into_mru(replacer, frame_id);
    lru_set_evictable(replacer, frame_id, true);

    return;
  }

  if (replacer->mru == frame) {
    return;
  }

  lru_do_remove(replacer, frame_id, false);
  lru_insert_into_mru(replacer, frame_id);
}


frame_id_t lru_debug_mru_at(struct lru *replacer, int idx) {
  struct frame *frame = replacer->mru;
  while (idx > 0) {
    if (frame) {
      frame = frame->next;
      idx--;
      continue;
    } 

    return INVALID_FRAME_ID; 
  } 

  if (frame) {
    return frame - replacer->frames;
  }

  return INVALID_FRAME_ID;
}

