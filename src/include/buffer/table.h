#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "buffer/frame.h"

struct entry {
  frame_id_t key;
  frame_id_t value; //TODO may need to change type later for clarity
};

struct bucket {
  uint32_t count;
  uint32_t capacity;
  struct entry *entries;
};

struct table {
  uint32_t bucket_count;
  uint32_t entry_count;  
  struct bucket *buckets;

  pthread_mutex_t latch;
};

void table_init(struct table *table, uint32_t bucket_count);
void table_free(struct table *table);
void table_put(struct table *table, frame_id_t key, frame_id_t value);
bool table_get(struct table *table, frame_id_t key, frame_id_t *value);
void table_delete(struct table *table, frame_id_t key);
uint32_t table_size(struct table *table);


#endif // TABLE_H
