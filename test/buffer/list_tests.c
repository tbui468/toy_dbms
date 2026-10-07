#include <stdio.h>
#include <stdbool.h>

#include "buffer/list.h"

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { total_tests++; tests_passed += (fcn)(); }

bool list_init_free(void) {
  struct list list;
  list_init(&list, 8);
  ASSERT_EQ((int) list.size, 0);
  ASSERT_EQ((int) list.capacity, 8);
  list_free(&list);
  return true;
}

bool list_append_within_capacity(void) {
  int cap = 8;
  struct list list;
  list_init(&list, cap);
  for (int i = 0; i < cap; i++) {
    list_append(&list, i);
    ASSERT_EQ((int) list.size, i + 1);
    ASSERT_EQ(list_contains(&list, i), 1);
  }
  list_free(&list);
  return true;
}

bool list_append_beyond_capacity(void) {
  int cap = 8;
  struct list list;
  list_init(&list, cap);
  for (int i = 0; i < cap * 10; i++) {
    list_append(&list, i);
    ASSERT_EQ((int) list.size, i + 1);
    ASSERT_EQ(list_contains(&list, i), 1);
  }
  list_free(&list);
  return true;
}

bool list_erase_within_capacity(void) {
  int cap = 8;
  struct list list;
  list_init(&list, cap);
  int offsets[8];
  for (int i = 0; i < cap; i++) {
    offsets[i] = list_append(&list, i);
    ASSERT_EQ((int) list.size, i + 1);
    ASSERT_EQ(list_contains(&list, i), 1);
  }

  for (int i = 0; i < cap; i++) {
    list_erase(&list, offsets[i]);
    ASSERT_EQ((int) list.size, 8 - (i + 1));
    ASSERT_EQ(!list_contains(&list, i), 1);
  }
  

  list_free(&list);
  return true;
}

bool list_erase_beyond_capacity(void) {
  int cap = 8;
  struct list list;
  list_init(&list, cap);
  int offsets[32];
  for (int i = 0; i < 32; i++) {
    offsets[i] = list_append(&list, i);
    ASSERT_EQ((int) list.size, i + 1);
    ASSERT_EQ(list_contains(&list, i), 1);
  }

  for (int i = 0; i < 32; i++) {
    list_erase(&list, offsets[i]);
    ASSERT_EQ((int) list.size, 32 - (i + 1));
    ASSERT_EQ(!list_contains(&list, i), 1);
  }
  

  list_free(&list);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(list_init_free);
  RUN_TEST(list_append_within_capacity);
  RUN_TEST(list_append_beyond_capacity);
  RUN_TEST(list_erase_within_capacity);
  RUN_TEST(list_erase_beyond_capacity);

  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
