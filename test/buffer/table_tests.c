#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "buffer/table.h"
#include "util/test.h"

bool table_init_free(void) {
  struct table table;
  table_init(&table, 8);
  ASSERT_EQ((int) table.entry_count, 0);
  ASSERT_EQ((int) table.bucket_count, 8);
  table_free(&table);
  return true;
}

bool table_put_within_threshold(void) {
  struct table table;
  table_init(&table, 256);

  //assuming threshold factor is some reasonable number < 16
  int count = 0;
  for (int i = 0; i < 20; i += 2) {
    table_put(&table, i, i);
    count++;
    ASSERT_EQ((int) table.entry_count, count);
  }

  for (int i = 0; i < 20; i += 2) {
    size_t value;
    ASSERT_EQ(table_get(&table, i, &value), 1);
  }

  table_free(&table);
  return true;
}

bool table_put_beyond_threshold(void) {
  struct table table;
  table_init(&table, 8);

  int count = 0;
  for (int i = 0; i < 256; i++) {
    table_put(&table, i, i);
    count++;
    ASSERT_EQ((int) table.entry_count, count);
  }

  for (int i = 0; i < 256; i++) {
    size_t value;
    ASSERT_EQ(table_get(&table, i, &value), 1);
  }

  table_free(&table);
  return true;
}

bool table_delete_beyond_threshold(void) {
  struct table table;
  table_init(&table, 8);

  for (int i = 0; i < 256; i++) {
    table_put(&table, i, i);
  }

  for (int i = 0; i < 256; i++) {
    size_t value;
    ASSERT_EQ(table_get(&table, i, &value), 1);
  }

  int count = 256;
  for (int i = 0; i < 256; i++) {
    table_delete(&table, i);
    count--;
    ASSERT_EQ((int) table.entry_count, count);
  }

  table_free(&table);
  return true;
}

bool table_put_duplicate_key(void) {
  struct table table;
  table_init(&table, 8);

  for (int i = 0; i < 256; i++) {
    table_put(&table, 0, i);
    size_t value;
    ASSERT_EQ(table_get(&table, 0, &value), 1);
    ASSERT_EQ((int) value, i);
    ASSERT_EQ((int) table.entry_count, 1);
  }

  return true;
}

#define COUNT 256 * 256

void *concurrency_helper0(void *arg) {
  struct table *table = (struct table *) arg;
  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 0) continue;
    table_put(table, i, i);
  }

  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 0) continue;
    size_t result;
    ASSERT_EQ(table_get(table, i, &result), 1);
  }

  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 0) continue;
    table_delete(table, i);
  }

  return NULL;
}

void *concurrency_helper1(void *arg) {
  struct table *table = (struct table *) arg;
  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 1) continue;
    table_put(table, i, i);
  }

  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 1) continue;
    size_t result;
    ASSERT_EQ(table_get(table, i, &result), 1);
  }

  for (int i = 0; i < COUNT; i++) {
    if (i % 2 == 1) continue;
    table_delete(table, i);
  }

  return NULL;
}

bool table_concurrency(void) {

  struct table table;
  table_init(&table, 256);
  
  pthread_t t0, t1;
 
  pthread_create(&t0, NULL, concurrency_helper0, &table); 
  pthread_create(&t1, NULL, concurrency_helper1, &table); 

  pthread_join(t0, NULL);
  pthread_join(t1, NULL);


  ASSERT_EQ(table_size(&table), 0);

  table_free(&table);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(table_init_free);
  RUN_TEST(table_put_within_threshold);
  RUN_TEST(table_put_beyond_threshold);
  RUN_TEST(table_delete_beyond_threshold);
  RUN_TEST(table_put_duplicate_key)
  RUN_TEST(table_concurrency)

  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
