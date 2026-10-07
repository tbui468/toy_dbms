#include <stdio.h>
#include <stdbool.h>

#include "buffer/table.h"

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { total_tests++; tests_passed += (fcn)(); }

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
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(table_init_free);
  RUN_TEST(table_put_within_threshold);
  RUN_TEST(table_put_beyond_threshold);
  RUN_TEST(table_delete_beyond_threshold);
  RUN_TEST(table_put_duplicate_key)

  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
