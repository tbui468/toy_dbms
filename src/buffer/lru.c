#include "buffer/lru.h"
#include <assert.h>
#include <stdio.h>

static void bucket_init(struct bucket *bucket) {
  bucket->count = 0;
  bucket->capacity = 0;
  bucket->entries = NULL;
}

static void bucket_free(struct bucket *bucket) {
  free(bucket->entries);
}

static void bucket_put(struct bucket *bucket, frame_id_t id, size_t offset) {
  if (bucket->count == bucket->capacity) {
      bucket->capacity = bucket->capacity == 0 ? 8 : bucket->capacity * 2;
      bucket->entries = realloc(bucket->entries, bucket->capacity * sizeof(struct entry));
  }

  bucket->entries[bucket->count] = (struct entry) { .id = id, .offset = offset };
  bucket->count++;
}

static bool bucket_get(struct bucket *bucket, frame_id_t id, size_t *offset) {
  for (int i = 0; i < bucket->count; i++) {
    if (bucket->entries[i].id == id) {
      *offset = bucket->entries[i].offset;
      return true;
    }
  }

  return false;
}

static void bucket_delete(struct bucket *bucket, frame_id_t id) {
  for (int i = 0; i < bucket->count; i++) {
    if (bucket->entries[i].id == id) {
      bucket->entries[i] = bucket->entries[bucket->count - 1];
      bucket->count--;
      return;
    }
  }

  assert(false);
}

static void table_init(struct table *table) {
  for (int i = 0; i < BUCKET_COUNT; i++) {
    bucket_init(table->buckets + i);
  }
}

static void table_free(struct table *table) {
  for (int i = 0; i < BUCKET_COUNT; i++) {
    bucket_free(table->buckets + i);
  }
}

static inline int hash(frame_id_t frame_id) {
  return frame_id % BUCKET_COUNT;
}

static void table_put(struct table *table, frame_id_t frame_id, size_t offset) {
  int bucket = hash(frame_id);
  size_t result;
  assert(!bucket_get(table->buckets + bucket, frame_id, &result));
  bucket_put(table->buckets + bucket, frame_id, offset);
}

static bool table_get(struct table *table, frame_id_t frame_id, size_t *offset) {
  int bucket = hash(frame_id);
  return bucket_get(table->buckets + bucket, frame_id, offset);
}

static void table_delete(struct table *table, frame_id_t frame_id) {
  int bucket = hash(frame_id);
  bucket_delete(table->buckets + bucket, frame_id);
}


void frame_init(struct frame *frame, frame_id_t id) {
  frame->next = INVALID_FRAME_OFFSET;
  frame->prev = INVALID_FRAME_OFFSET;
  frame->id = id;
}

static inline struct frame* frame_at(struct lru *replacer, size_t offset) {
  assert(offset < replacer->capacity); // TODO remove capacity later
  return &replacer->frames[offset];
}

static inline size_t frame_offset(struct lru *replacer, frame_id_t frame_id) {
  size_t offset;
  bool result = table_get(&replacer->table, frame_id, &offset);
  return result ? offset : INVALID_FRAME_OFFSET;
}

void lru_init(struct lru *replacer, int capacity) {
  replacer->capacity = capacity;
  replacer->frames = malloc(sizeof(struct frame) * replacer->capacity);
  for (int i = 0; i < replacer->capacity; i++) {
    frame_init(replacer->frames + i, INVALID_FRAME_ID);
  }
  replacer->size = 0;
  replacer->lru = INVALID_FRAME_OFFSET;
  replacer->mru = INVALID_FRAME_OFFSET;

  table_init(&replacer->table);
}

void lru_free(struct lru *replacer) {
  free(replacer->frames);
  table_free(&replacer->table);
}

int lru_size(struct lru *replacer) {
  return replacer->size;
}

int lru_capacity(struct lru *replacer) {
  return replacer->capacity;
}

void lru_remove(struct lru *replacer, frame_id_t frame_id) {
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
static bool lru_frame_unfixed(struct lru *replacer, frame_id_t frame_id) {
  size_t offset = frame_offset(replacer, frame_id);
  return offset != INVALID_FRAME_OFFSET;
}

void lru_fix(struct lru *replacer, frame_id_t frame_id) {
  if (lru_frame_unfixed(replacer, frame_id)) {
    lru_remove(replacer, frame_id);
  }
}

void lru_unfix(struct lru *replacer, frame_id_t frame_id) {
  assert(!lru_frame_unfixed(replacer, frame_id)); // calling unfix on already unfixed frame doesn't make sense

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

frame_id_t lru_evict(struct lru *replacer) {
  if (replacer->lru != INVALID_FRAME_OFFSET) {
    struct frame *frame = frame_at(replacer, replacer->lru);
    frame_id_t frame_id = frame->id;
    lru_remove(replacer, frame_id);
    frame_init(frame, INVALID_FRAME_ID);
    return frame_id;
  }

  return INVALID_FRAME_ID;
}

frame_id_t lru_debug_mru_at(struct lru *replacer, int idx) {
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

