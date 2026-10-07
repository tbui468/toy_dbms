#include <stdio.h>
#include <stdbool.h>

#include "buffer/bpm.h"

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { total_tests++; tests_passed += (fcn)(); }


int main(void) {
  int tests_passed = 0;
  int total_tests = 0;

  /*
  RUN_TEST(lru_init_free);
  RUN_TEST(lru_first_access);
  RUN_TEST(lru_unfix_order);
  RUN_TEST(lru_evict_frame);
  RUN_TEST(lru_evict_and_reuse);
  RUN_TEST(lru_scale);
  */
 
  printf("%d/%d tests passed!\n", tests_passed, total_tests);
}
