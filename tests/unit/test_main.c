#include <stdio.h>

int test_stats(void);
int test_registry(void);

int main(void) {
  int failures = 0;
  failures += test_stats();
  failures += test_registry();
  if (failures == 0) {
    printf("all tests passed\n");
    return 0;
  }
  printf("%d tests failed\n", failures);
  return 1;
}
