#include "buffer/table.h"
#include <stdio.h>

static void bucket_init(struct bucket *bucket) {
  bucket->count = 0;
  bucket->capacity = 0;
  bucket->entries = NULL;
}

static void bucket_free(struct bucket *bucket) {
  free(bucket->entries);
}

static bool bucket_put(struct bucket *bucket, size_t key, size_t value) {
  for (size_t i = 0; i < bucket->count; i++) {
    if (bucket->entries[i].key == key) {
      bucket->entries[i].value = value;
      return false;
    }
  }

  if (bucket->count == bucket->capacity) {
      bucket->capacity = bucket->capacity == 0 ? 8 : bucket->capacity * 2;
      bucket->entries = realloc(bucket->entries, bucket->capacity * sizeof(struct entry));
  }

  bucket->entries[bucket->count] = (struct entry) { .key = key, .value = value };
  bucket->count++;
  return true;
}

static bool bucket_get(struct bucket *bucket, size_t key, size_t *value) {
  for (size_t i = 0; i < bucket->count; i++) {
    if (bucket->entries[i].key == key) {
      *value = bucket->entries[i].value;
      return true;
    }
  }

  return false;
}

static void bucket_delete(struct bucket *bucket, size_t key) {
  for (size_t i = 0; i < bucket->count; i++) {
    if (bucket->entries[i].key == key) {
      bucket->entries[i] = bucket->entries[bucket->count - 1];
      bucket->count--;
      return;
    }
  }

  assert(false);
}

void table_init(struct table *table, size_t bucket_count) {
  table->bucket_count = bucket_count;
  table->entry_count = 0;
  table->buckets = malloc(bucket_count * sizeof(struct bucket));
  for (size_t i = 0; i < bucket_count; i++) {
    bucket_init(table->buckets + i);
  }

  pthread_mutex_init(&table->latch, NULL);
}

void table_free(struct table *table) {
  for (size_t i = 0; i < table->bucket_count; i++) {
    bucket_free(table->buckets + i);
  }
  free(table->buckets);
  pthread_mutex_destroy(&table->latch);
}

static inline int hash(struct table *table, size_t key) {
  return key % table->bucket_count;
}

static void table_resize(struct table *old_table) {
  struct table new_table;
  table_init(&new_table, old_table->bucket_count * 2);

  for (size_t i = 0; i < old_table->bucket_count; i++) {
    struct bucket *bucket = &old_table->buckets[i];
    for (size_t j = 0; j < bucket->count; j++) {
      size_t key = bucket->entries[j].key;
      size_t value = bucket->entries[j].value;
      table_put(&new_table, key, value);
    }
  }

  table_free(old_table);
  old_table->bucket_count = new_table.bucket_count;
  old_table->entry_count = new_table.entry_count;
  old_table->buckets = new_table.buckets;
}

void table_put(struct table *table, size_t key, size_t value) {
  pthread_mutex_lock(&table->latch);
  int bucket = hash(table, key);
  table->entry_count += bucket_put(table->buckets + bucket, key, value);

  const float LOAD_FACTOR_THRESHOLD = 2.0f;
  if (table->entry_count > table->bucket_count * LOAD_FACTOR_THRESHOLD) {
    table_resize(table);
  }
  pthread_mutex_unlock(&table->latch);
}

bool table_get(struct table *table, size_t key, size_t *value) {
  pthread_mutex_lock(&table->latch);
  int bucket = hash(table, key);
  bool result = bucket_get(table->buckets + bucket, key, value);
  pthread_mutex_unlock(&table->latch);
  return result;
}

void table_delete(struct table *table, size_t key) {
  pthread_mutex_lock(&table->latch);
  int bucket = hash(table, key);
  bucket_delete(table->buckets + bucket, key);
  table->entry_count--;
  pthread_mutex_unlock(&table->latch);
}

int table_size(struct table *table) {
  pthread_mutex_lock(&table->latch);
  size_t size = table->entry_count;
  pthread_mutex_unlock(&table->latch);
  return size;
}
