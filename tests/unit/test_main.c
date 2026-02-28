#include <stdio.h>

int test_stats(void);
int test_registry(void);
int test_system(void);
int test_system_linux_parse(void);

int main(void) {
  int failures = 0;
  failures += test_stats();
  failures += test_registry();
  failures += test_system();
  failures += test_system_linux_parse();
  if (failures == 0) {
    printf("all tests passed\n");
    return 0;
  }
  printf("%d tests failed\n", failures);
  return 1;
}
