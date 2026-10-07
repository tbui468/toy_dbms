#include <stdio.h>
#include <stdbool.h>

#include "buffer/replacer.h"

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { printf("Running %s\n", #fcn); total_tests++; tests_passed += (fcn)(); }

bool replacer_init_free(void) {
  int cap = 8;
  struct replacer replacer;
  replacer_init(&replacer, cap);
  ASSERT_EQ(replacer_size(&replacer), 0);
  replacer_free(&replacer);
  return true;
}

bool replacer_first_access(void) {
  int cap = 8;
  struct replacer replacer;
  replacer_init(&replacer, cap);

  replacer_fix(&replacer, 0);
  ASSERT_EQ(replacer_size(&replacer), 0);
  replacer_fix(&replacer, 1);
  ASSERT_EQ(replacer_size(&replacer), 0);
  replacer_fix(&replacer, 2);
  ASSERT_EQ(replacer_size(&replacer), 0);

  replacer_free(&replacer);
  return true;
}
bool replacer_unfix_order(void) {
  int cap = 8;
  struct replacer replacer;
  replacer_init(&replacer, cap);

  //access 3 frames
  {
    replacer_fix(&replacer, 1);
    replacer_fix(&replacer, 2);
    replacer_fix(&replacer, 3);

    replacer_unfix(&replacer, 1);
    ASSERT_EQ(replacer_size(&replacer), 1);
    replacer_unfix(&replacer, 3);
    ASSERT_EQ(replacer_size(&replacer), 2);
    replacer_unfix(&replacer, 2);
    ASSERT_EQ(replacer_size(&replacer), 3);
  }

  replacer_free(&replacer);
  return true;
}

bool replacer_evict_frame(void) {
  int cap = 8;
  struct replacer replacer;
  replacer_init(&replacer, cap);

  replacer_fix(&replacer, 1);
  replacer_fix(&replacer, 2);
  replacer_fix(&replacer, 3);
  ASSERT_EQ(replacer_size(&replacer), 0);

  replacer_unfix(&replacer, 3);
  replacer_unfix(&replacer, 1);
  replacer_unfix(&replacer, 2);

  ASSERT_EQ(replacer_size(&replacer), 3);

  ASSERT_EQ(replacer_evict(&replacer), 3);
  ASSERT_EQ(replacer_size(&replacer), 2);
  ASSERT_EQ(replacer_evict(&replacer), 1);
  ASSERT_EQ(replacer_size(&replacer), 1);
  ASSERT_EQ(replacer_evict(&replacer), 2);
  ASSERT_EQ(replacer_size(&replacer), 0);

  replacer_free(&replacer);
  return true;
}

bool replacer_evict_and_reuse(void) {
  int cap = 8;
  struct replacer replacer;
  replacer_init(&replacer, cap);

  replacer_fix(&replacer, 1);
  replacer_fix(&replacer, 3);
  replacer_fix(&replacer, 2);

  replacer_unfix(&replacer, 1);
  ASSERT_EQ(replacer_evict(&replacer), 1);
  replacer_fix(&replacer, 1);
  ASSERT_EQ(replacer_evict(&replacer), INVALID_FRAME_ID);

  replacer_free(&replacer);
  return true;
}

bool replacer_scale(void) {
  int cap = 1024 * 1024;
  struct replacer replacer;
  replacer_init(&replacer, cap);
  
  for (int i = 0; i < cap; i++) {
    replacer_fix(&replacer, i);
    if (i % 2 == 0) {
      replacer_unfix(&replacer, i);
    }
  }

  for (int i = 0; i < cap / 2; i++) {
    ASSERT_EQ(replacer_evict(&replacer), i * 2);
  }
  

  replacer_free(&replacer);
  return true;
}

int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  RUN_TEST(replacer_init_free);
  RUN_TEST(replacer_first_access);
  RUN_TEST(replacer_unfix_order);
  RUN_TEST(replacer_evict_frame);
  RUN_TEST(replacer_evict_and_reuse);
  RUN_TEST(replacer_scale);

  //  concurrent_test /* see how this is tested in bustub ARC to ensure we test correctly */
  //      have 2 or more threads regularly record/evict unique frames
  //      ensure that resulting frames is not corrupted due to sharing of replacer
 
  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
