#ifndef TABLE_H
#define TABLE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

struct entry {
  size_t key;
  size_t value;
};

struct bucket {
  size_t count;
  size_t capacity;
  struct entry *entries;
};

struct table {
  size_t bucket_count;
  size_t entry_count;  
  struct bucket *buckets;

  pthread_mutex_t latch;
};

void table_init(struct table *table, size_t bucket_count);
void table_free(struct table *table);
void table_put(struct table *table, size_t key, size_t value);
bool table_get(struct table *table, size_t key, size_t *value);
void table_delete(struct table *table, size_t key);
int table_size(struct table *table);


#endif // TABLE_H
