#include "buffer/replacer.h"
#include <assert.h>
#include <stdio.h>

void replacer_init(struct replacer *replacer, int capacity) {
  replacer->capacity = capacity;
  list_init(&replacer->list, capacity);
  table_init(&replacer->table, capacity);
  pthread_mutex_init(&replacer->latch, NULL);
}

void replacer_free(struct replacer *replacer) {
  table_free(&replacer->table);
  list_free(&replacer->list);
  pthread_mutex_destroy(&replacer->latch);
}

int replacer_size(struct replacer *replacer) {
  return list_size(&replacer->list);
}


void replacer_fix(struct replacer *replacer, frame_id_t frame_id) {
  pthread_mutex_lock(&replacer->latch);
  size_t off;
  if (table_get(&replacer->table, frame_id, &off)) {
    table_delete(&replacer->table, frame_id);
    list_erase(&replacer->list, off);
  }
  pthread_mutex_unlock(&replacer->latch);
}

void replacer_unfix(struct replacer *replacer, frame_id_t frame_id) {
  pthread_mutex_lock(&replacer->latch);
  size_t throwaway;
  assert(!table_get(&replacer->table, frame_id, &throwaway));
  
  size_t off = list_append(&replacer->list, frame_id);
  table_put(&replacer->table, frame_id, off);
  pthread_mutex_unlock(&replacer->latch);
}

frame_id_t replacer_evict(struct replacer *replacer) {
  pthread_mutex_lock(&replacer->latch);
  if (list_size(&replacer->list) == 0) {
    pthread_mutex_unlock(&replacer->latch);
    return INVALID_FRAME_ID;
  }

  frame_id_t frame_id = list_pop_front(&replacer->list); 
  table_delete(&replacer->table, frame_id);
  pthread_mutex_unlock(&replacer->latch);
  return frame_id;
}

