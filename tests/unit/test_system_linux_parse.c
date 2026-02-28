#include <stdio.h>
#include <string.h>

#if defined(__linux__)
int hwb_parse_linux_cpuinfo_stream(FILE* f, char* cpu_model, size_t cpu_model_size, int* logical_cores, int* physical_cores);

static int run_parse_case(const char* cpuinfo, const char* expected_model, int expected_logical, int expected_physical) {
  FILE* f = tmpfile();
  if (!f) {
    puts("tmpfile failed");
    return 1;
  }
  if (fputs(cpuinfo, f) == EOF) {
    puts("fputs failed");
    fclose(f);
    return 1;
  }
  rewind(f);

  char model[128] = {0};
  int logical = 0;
  int physical = 0;
  int rc = hwb_parse_linux_cpuinfo_stream(f, model, sizeof(model), &logical, &physical);
  fclose(f);
  if (rc != 0) {
    puts("hwb_parse_linux_cpuinfo_stream failed");
    return 1;
  }
  if (logical != expected_logical) {
    puts("unexpected logical core count");
    return 1;
  }
  if (physical != expected_physical) {
    puts("unexpected physical core count");
    return 1;
  }
  if (expected_model && strcmp(model, expected_model) != 0) {
    puts("unexpected cpu model");
    return 1;
  }
  return 0;
}
#endif

int test_system_linux_parse(void) {
#if defined(__linux__)
  const char* multisocket_smt =
      "processor\t: 0\n"
      "model name\t: Example CPU\n"
      "physical id\t: 0\n"
      "core id\t\t: 0\n"
      "\n"
      "processor\t: 1\n"
      "physical id\t: 0\n"
      "core id\t\t: 0\n"
      "\n"
      "processor\t: 2\n"
      "physical id\t: 1\n"
      "core id\t\t: 0\n"
      "\n"
      "processor\t: 3\n"
      "physical id\t: 1\n"
      "core id\t\t: 1\n";
  if (run_parse_case(multisocket_smt, "Example CPU", 4, 3) != 0) return 1;

  const char* missing_fields =
      "processor\t: 0\n"
      "model name\t: Example CPU\n"
      "\n"
      "processor\t: 1\n";
  if (run_parse_case(missing_fields, "Example CPU", 2, 0) != 0) return 1;

  const char* unusual_order =
      "core id\t\t: 2\n"
      "physical id\t: 5\n"
      "model name\t: Example CPU\n"
      "processor\t: 0\n"
      "\n"
      "physical id\t: 5\n"
      "core id\t\t: 3\n"
      "processor\t: 1\n";
  if (run_parse_case(unusual_order, "Example CPU", 2, 2) != 0) return 1;
#endif
  return 0;
}
