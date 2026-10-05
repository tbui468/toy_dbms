#include <stdio.h>
#include <stdbool.h>

#include "buffer/lru.h"

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { total_tests++; tests_passed += (fcn)(); }

bool lru_init_free(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);
  ASSERT_EQ(lru_capacity(&replacer), cap);
  ASSERT_EQ(lru_size(&replacer), 0);
  lru_free(&replacer);
  return true;
}

bool lru_first_access(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);
  ASSERT_EQ(lru_size(&replacer), 0);
  lru_record_access(&replacer, 1);
  ASSERT_EQ(lru_size(&replacer), 1);
  lru_record_access(&replacer, 2);
  ASSERT_EQ(lru_size(&replacer), 2);
  lru_record_access(&replacer, 3);
  ASSERT_EQ(lru_size(&replacer), 3);

  lru_set_evictable(&replacer, 1, false);
  ASSERT_EQ(lru_size(&replacer), 2);
  lru_set_evictable(&replacer, 2, false);
  ASSERT_EQ(lru_size(&replacer), 1);
  lru_set_evictable(&replacer, 3, false);
  ASSERT_EQ(lru_size(&replacer), 0);

  lru_free(&replacer);
  return true;
}

bool lru_access_order(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);

  //access 3 frames
  {
    lru_record_access(&replacer, 1);
    lru_record_access(&replacer, 3);
    lru_record_access(&replacer, 2);

    frame_id_t frame_ids[] = { 2, 3, 1 };
    for (int i = 0; i < 3; i++) {
      ASSERT_EQ(lru_debug_mru_at(&replacer, i), frame_ids[i]); 
    }
  }

  //access middle frame
  {
    lru_record_access(&replacer, 3);

    frame_id_t frame_ids[] = { 3, 2, 1 };
    for (int i = 0; i < 3; i++) {
      ASSERT_EQ(lru_debug_mru_at(&replacer, i), frame_ids[i]); 
    }
  }

  //access mru frame
  {
    lru_record_access(&replacer, 3);

    frame_id_t frame_ids[] = { 3, 2, 1 };
    for (int i = 0; i < 3; i++) {
      ASSERT_EQ(lru_debug_mru_at(&replacer, i), frame_ids[i]); 
    }
  }

  //access lru frame
  {
    lru_record_access(&replacer, 1);

    frame_id_t frame_ids[] = { 1, 3, 2 };
    for (int i = 0; i < 3; i++) {
      ASSERT_EQ(lru_debug_mru_at(&replacer, i), frame_ids[i]); 
    }
  }

  lru_free(&replacer);
  return true;
}


bool lru_evict_frame(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);

  lru_record_access(&replacer, 1);
  lru_record_access(&replacer, 3);
  lru_record_access(&replacer, 2);
  ASSERT_EQ(lru_evict(&replacer), 1);
  ASSERT_EQ(lru_size(&replacer), 2);

  frame_id_t frame_ids[] = { 2, 3 };
  for (int i = 0; i < 2; i++) {
    ASSERT_EQ(lru_debug_mru_at(&replacer, i), frame_ids[i]); 
  }

  ASSERT_EQ(lru_evict(&replacer), 3);
  ASSERT_EQ(lru_evict(&replacer), 2);
  ASSERT_EQ(lru_evict(&replacer), INVALID_FRAME_ID);
  ASSERT_EQ(lru_size(&replacer), 0);

  lru_free(&replacer);
  return true;
}

bool lru_skip_non_evictable(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);

  lru_record_access(&replacer, 1);
  lru_record_access(&replacer, 3);
  lru_record_access(&replacer, 2);
  lru_set_evictable(&replacer, 1, false);

  ASSERT_EQ(lru_evict(&replacer), 3);
  ASSERT_EQ(lru_size(&replacer), 1);
  ASSERT_EQ(lru_evict(&replacer), 2);
  ASSERT_EQ(lru_size(&replacer), 0);
  ASSERT_EQ(lru_evict(&replacer), INVALID_FRAME_ID);
  ASSERT_EQ(lru_size(&replacer), 0);

  lru_set_evictable(&replacer, 1, true);
  ASSERT_EQ(lru_size(&replacer), 1);
  ASSERT_EQ(lru_evict(&replacer), 1);
  ASSERT_EQ(lru_size(&replacer), 0);

  lru_free(&replacer);
  return true;
}

bool lru_remove_non_evictable(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);

  lru_record_access(&replacer, 1);
  lru_record_access(&replacer, 3);
  lru_record_access(&replacer, 2);
  lru_set_evictable(&replacer, 1, false);
  lru_set_evictable(&replacer, 2, false);

  ASSERT_EQ(lru_size(&replacer), 1);
  lru_remove(&replacer, 1);
  ASSERT_EQ(lru_size(&replacer), 1);

  lru_remove(&replacer, 3);
  ASSERT_EQ(lru_size(&replacer), 0);

  lru_remove(&replacer, 2);
  ASSERT_EQ(lru_size(&replacer), 0);

  ASSERT_EQ(lru_debug_mru_at(&replacer, 0), INVALID_FRAME_ID);

  lru_free(&replacer);
  return true;
}

bool lru_evict_and_reuse(void) {
  int cap = 8;
  struct lru replacer;
  lru_init(&replacer, cap);

  lru_record_access(&replacer, 1);
  lru_record_access(&replacer, 3);
  lru_record_access(&replacer, 2);

  ASSERT_EQ(lru_evict(&replacer), 1);
  lru_record_access(&replacer, 1);
  ASSERT_EQ(lru_evict(&replacer), 3);
  ASSERT_EQ(lru_evict(&replacer), 2);
  ASSERT_EQ(lru_evict(&replacer), 1);

  lru_free(&replacer);
  return true;
}

bool lru_scale(void) {
  int cap = 1024 * 1024;
  struct lru replacer;
  lru_init(&replacer, cap);
  
  for (int i = 0; i < cap; i++) {
    lru_record_access(&replacer, i);
    lru_set_evictable(&replacer, i, i % 2 == 0);
  }

  for (int i = 0; i < cap / 2; i++) {
    if (i % 1000 == 0) printf("%d\n", i);
    ASSERT_EQ(lru_evict(&replacer), i * 2);
  }
  

  lru_free(&replacer);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(lru_init_free);
  RUN_TEST(lru_first_access);
  RUN_TEST(lru_access_order);
  RUN_TEST(lru_evict_frame);
  RUN_TEST(lru_skip_non_evictable);
  RUN_TEST(lru_remove_non_evictable);
  RUN_TEST(lru_evict_and_reuse);
  RUN_TEST(lru_scale);

  //  scale_test large capacity (1 million pages)
  //  concurrent_test /* see how this is tested in bustub ARC to ensure we test correctly */
  //      have 2 or more threads regularly record/evict unique frames
  //      ensure that resulting frames is not corrupted due to sharing of replacer
 
  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
