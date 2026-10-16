#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "buffer/list.h"
#include "util/test.h"

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

bool list_erase_within_capacity(void) {
  int cap = 8;
  struct list list;
  list_init(&list, cap);

  for (int i = 0; i < cap; i++) {
    list_append(&list, i);
    ASSERT_EQ((int) list.size, i + 1);
    ASSERT_EQ(list_contains(&list, i), 1);
  }

  for (int i = 0; i < cap; i++) {
    list_erase(&list, i);
    ASSERT_EQ((int) list.size, 8 - (i + 1));
    ASSERT_EQ(!list_contains(&list, i), 1);
  }
  

  list_free(&list);
  return true;
}

#define COUNT 256 * 256

void *concurrency_helper0(void *arg) {
  struct list *list = (struct list *) arg;
  for (size_t i = 0; i < COUNT; i++) {
    if (i % 2 == 0) continue;
    list_append(list, i);
  }
  return NULL;
}

void *concurrency_helper1(void *arg) {
  struct list *list = (struct list *) arg;
  for (size_t i = 0; i < COUNT; i++) {
    if (i % 2 == 1) continue;
    list_append(list, i);
  }
  return NULL;
}

void *delete_helper(void *arg) {
  struct list *list = (struct list *) arg;
  while (list_size(list) > 0) {
    list_pop_front(list);
  }
  return NULL;
}

bool list_concurrency(void) {
  struct list list;
  list_init(&list, COUNT);
  
  pthread_t t0, t1;
 
  pthread_create(&t0, NULL, concurrency_helper0, &list); 
  pthread_create(&t1, NULL, concurrency_helper1, &list); 

  pthread_join(t0, NULL);
  pthread_join(t1, NULL);

  for (size_t i = 0; i < COUNT; i++) {
    ASSERT_EQ(list_contains(&list, i), 1);
  }
  ASSERT_EQ(list_size(&list), COUNT);

  pthread_create(&t0, NULL, delete_helper, &list); 
  pthread_create(&t1, NULL, delete_helper, &list); 

  pthread_join(t0, NULL);
  pthread_join(t1, NULL);

  ASSERT_EQ(list_size(&list), 0);

  list_free(&list);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(list_init_free);
  RUN_TEST(list_append_within_capacity);
  RUN_TEST(list_erase_within_capacity);
  RUN_TEST(list_concurrency);

  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
