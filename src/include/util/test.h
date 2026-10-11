#ifndef TEST_H
#define TEST_H

#define ASSERT_EQ(a, b) if ((a) != (b)) { printf("[%d] Got %d but expected %d\n", __LINE__, (a), (b)); return false; }
#define RUN_TEST(fcn) { printf("Running %s\n", #fcn); fflush(stdout); total_tests++; int result = (fcn)(); tests_passed += result; printf(result ? "    passed\n" : "    failed\n"); fflush(stdout); }

#endif // TEST_H
