#include "buffer/replacer.h"
#include "util/util.h"
#include <assert.h>
#include <stdio.h>

void replacer_init(struct replacer *replacer, uint32_t capacity) {
  replacer->capacity = capacity;
  list_init(&replacer->list, capacity);
  table_init(&replacer->table, capacity);
  latch_init(&replacer->latch);
}

void replacer_free(struct replacer *replacer) {
  table_free(&replacer->table);
  list_free(&replacer->list);
  latch_destroy(&replacer->latch);
}

uint32_t replacer_size(struct replacer *replacer) {
  return list_size(&replacer->list);
}


void replacer_fix(struct replacer *replacer, frame_id_t frame_id) {
  latch_lock(&replacer->latch);
  frame_id_t off;
  if (table_get(&replacer->table, frame_id, &off)) {
    assert(list_contains(&replacer->list, frame_id));
    table_delete(&replacer->table, frame_id);
    list_erase(&replacer->list, off);
  }
  latch_unlock(&replacer->latch);
}

void replacer_unfix(struct replacer *replacer, frame_id_t frame_id) {
  latch_lock(&replacer->latch);
  frame_id_t throwaway;
  assert(!table_get(&replacer->table, frame_id, &throwaway));
  assert(!list_contains(&replacer->list, frame_id));
  
  list_append(&replacer->list, frame_id);
  table_put(&replacer->table, frame_id, frame_id);
  latch_unlock(&replacer->latch);
}

frame_id_t replacer_evict(struct replacer *replacer) {
  latch_lock(&replacer->latch);
  if (list_size(&replacer->list) == 0) {
    latch_unlock(&replacer->latch);
    return INVALID_FRAME_ID;
  }

  frame_id_t frame_id = list_pop_front(&replacer->list); 
  table_delete(&replacer->table, frame_id);
  latch_unlock(&replacer->latch);
  return frame_id;
}

