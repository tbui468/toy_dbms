#include "buffer/replacer.h"
#include <assert.h>
#include <stdio.h>


void frame_init(struct frame *frame, frame_id_t id) {
  frame->next = INVALID_FRAME_OFFSET;
  frame->prev = INVALID_FRAME_OFFSET;
  frame->id = id;
}

static inline struct frame* frame_at(struct replacer *replacer, size_t offset) {
  assert(offset < replacer->capacity); // TODO remove capacity later
  return &replacer->frames[offset];
}

static inline size_t frame_offset(struct replacer *replacer, frame_id_t frame_id) {
  size_t offset;
  bool result = table_get(&replacer->table, frame_id, &offset);
  return result ? offset : INVALID_FRAME_OFFSET;
}

void replacer_init(struct replacer *replacer, int capacity) {
  replacer->capacity = capacity;
  replacer->frames = malloc(sizeof(struct frame) * replacer->capacity);
  for (int i = 0; i < replacer->capacity; i++) {
    frame_init(replacer->frames + i, INVALID_FRAME_ID);
  }
  replacer->size = 0;
  replacer->lru = INVALID_FRAME_OFFSET;
  replacer->mru = INVALID_FRAME_OFFSET;

  table_init(&replacer->table, capacity);
}

void replacer_free(struct replacer *replacer) {
  free(replacer->frames);
  table_free(&replacer->table);
}

int replacer_size(struct replacer *replacer) {
  return replacer->size;
}

int replacer_capacity(struct replacer *replacer) {
  return replacer->capacity;
}

void replacer_remove(struct replacer *replacer, frame_id_t frame_id) {
  struct frame *frame = frame_at(replacer, frame_offset(replacer, frame_id));

  if (frame->prev != INVALID_FRAME_OFFSET) {
    struct frame *prev = frame_at(replacer, frame->prev);
    prev->next = frame->next;
  } else {
    replacer->mru = frame->next;  
  }

  if (frame->next != INVALID_FRAME_OFFSET) {
    struct frame *next = frame_at(replacer, frame->next);
    next->prev = frame->prev;
  } else {
    replacer->lru = frame->prev; 
  }

  replacer->size--;

  table_delete(&replacer->table, frame_id);
}
  
// the bpm handles whether a frame is fixed or free
static bool replacer_frame_unfixed(struct replacer *replacer, frame_id_t frame_id) {
  size_t offset = frame_offset(replacer, frame_id);
  return offset != INVALID_FRAME_OFFSET;
}

void replacer_fix(struct replacer *replacer, frame_id_t frame_id) {
  if (replacer_frame_unfixed(replacer, frame_id)) {
    replacer_remove(replacer, frame_id);
  }
}

void replacer_unfix(struct replacer *replacer, frame_id_t frame_id) {
  assert(!replacer_frame_unfixed(replacer, frame_id)); // calling unfix on already unfixed frame doesn't make sense

  size_t offset = frame_offset(replacer, frame_id);
  assert(offset == INVALID_FRAME_OFFSET);
  // TODO just allocating frame_id == offset for now - need to use proper allocator with freelist
  struct frame *frame = frame_at(replacer, frame_id);
  offset = frame_id;
  //end allocation
  frame_init(frame, frame_id);
  replacer->size++;
  frame->prev = INVALID_FRAME_OFFSET;

  size_t prev_mru_offset = replacer->mru;
  frame->next = replacer->mru;

  if (prev_mru_offset != INVALID_FRAME_OFFSET) {
    struct frame *prev_mru = frame_at(replacer, prev_mru_offset);
    prev_mru->prev = offset;
  }

  if (replacer->lru == INVALID_FRAME_OFFSET) {
    replacer->lru = offset;
  }

  replacer->mru = offset;

  table_put(&replacer->table, frame_id, offset);
}

frame_id_t replacer_evict(struct replacer *replacer) {
  if (replacer->lru != INVALID_FRAME_OFFSET) {
    struct frame *frame = frame_at(replacer, replacer->lru);
    frame_id_t frame_id = frame->id;
    replacer_remove(replacer, frame_id);
    frame_init(frame, INVALID_FRAME_ID);
    return frame_id;
  }

  return INVALID_FRAME_ID;
}

frame_id_t replacer_debug_mru_at(struct replacer *replacer, int idx) {
  size_t offset = replacer->mru;
  while (idx > 0 && offset != INVALID_FRAME_OFFSET) {
    struct frame *frame = frame_at(replacer, offset);
    offset = frame->next;
    idx--;
  } 

  if (offset != INVALID_FRAME_OFFSET) {
    struct frame *frame = frame_at(replacer, offset);
    return frame->id;
  }

  return INVALID_FRAME_ID;
}

