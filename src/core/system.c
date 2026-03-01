#include "hwbench/system.h"
#include "hwbench/system_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <sys/mount.h>
  #include <sys/sysctl.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <fcntl.h>
#else
  #include <dirent.h>
  #include <sys/statvfs.h>
#endif

static void hwb_copy_string(char* dst, size_t dst_size, const char* src) {
  if (!dst || dst_size == 0) {
    return;
  }
  if (!src) {
    dst[0] = '\0';
    return;
  }
  size_t i = 0;
  for (; i + 1 < dst_size && src[i] != '\0'; ++i) {
    dst[i] = src[i];
  }
  dst[i] = '\0';
}

static void hwb_trim_newline(char* s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
    s[--n] = '\0';
  }
}

static void hwb_append_csv_token(char* dst, size_t dst_size, const char* token) {
  if (!dst || dst_size == 0 || !token || token[0] == '\0') return;
  size_t used = strlen(dst);
  if (used >= dst_size - 1) return;
  if (used != 0) {
    if (used + 2 >= dst_size) return;
    dst[used++] = ',';
    dst[used++] = ' ';
    dst[used] = '\0';
  }
  size_t i = 0;
  while (used + i + 1 < dst_size && token[i] != '\0') {
    dst[used + i] = token[i];
    ++i;
  }
  dst[used + i] = '\0';
}

const char* hwb_os_name(void) {
#if defined(_WIN32)
  return "windows";
#elif defined(__APPLE__)
  return "apple";
#elif defined(__linux__)
  return "linux";
#else
  return "unknown";
#endif
}

const char* hwb_arch_name(void) {
#if defined(__x86_64__) || defined(_M_X64)
  return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
  return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
  return "arm";
#else
  return "unknown";
#endif
}

#if defined(__linux__)

/* A (physical_id, core_id) pair used to count unique physical cores. */
typedef struct {
  int phys;
  int core;
} HwbCorePair;

enum { HWB_INITIAL_CORE_PAIR_CAPACITY = 64 };

static int hwb_pair_seen(const HwbCorePair* pairs, int count, int phys, int core) {
  for (int i = 0; i < count; ++i) {
    if (pairs[i].phys == phys && pairs[i].core == core) return 1;
  }
  return 0;
}

static int hwb_add_pair(HwbCorePair** pairs, int* pair_count, int* pair_cap, int phys, int core) {
  if (hwb_pair_seen(*pairs, *pair_count, phys, core)) return 0;
  if (*pair_count == *pair_cap) {
    size_t grown_cap = *pair_cap > 0 ? ((size_t)(*pair_cap) * 2U) : (size_t)HWB_INITIAL_CORE_PAIR_CAPACITY;
    if (grown_cap > (SIZE_MAX / sizeof(HwbCorePair)) || grown_cap > (size_t)INT_MAX) return -1;
    int new_cap = (int)grown_cap;
    HwbCorePair* grown = (HwbCorePair*)realloc(*pairs, (size_t)new_cap * sizeof(HwbCorePair));
    if (!grown) {
      return -1;
    }
    *pairs = grown;
    *pair_cap = new_cap;
  }
  (*pairs)[*pair_count].phys = phys;
  (*pairs)[*pair_count].core = core;
  ++(*pair_count);
  return 0;
}

static void hwb_disable_pair_tracking(HwbCorePair** pairs, int* pair_count, int* pair_cap, int* can_track_pairs) {
  if (pairs && *pairs) {
    free(*pairs);
    *pairs = NULL;
  }
  if (pair_count) *pair_count = 0;
  if (pair_cap) *pair_cap = 0;
  if (can_track_pairs) *can_track_pairs = 0;
}

int hwb_parse_linux_cpuinfo_stream(FILE* f, char* cpu_model, size_t cpu_model_size, int* logical_cores, int* physical_cores) {
  if (!f || !logical_cores || !physical_cores) return -1;

  HwbCorePair* pairs = NULL;
  int pair_count = 0;
  int pair_cap = 0;
  int can_track_pairs = 1;
  char line[512];
  int logical = 0;
  int cur_phys = -1;
  int cur_core = -1;

  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "model name", 10) == 0 && cpu_model && cpu_model_size > 0 && cpu_model[0] == '\0') {
      char* p = strchr(line, ':');
      if (p) {
        p += 1;
        while (*p == ' ' || *p == '\t') ++p;
        hwb_trim_newline(p);
        hwb_copy_string(cpu_model, cpu_model_size, p);
      }
    } else if (strncmp(line, "processor", 9) == 0) {
      if (can_track_pairs && cur_phys >= 0 && cur_core >= 0) {
        if (hwb_add_pair(&pairs, &pair_count, &pair_cap, cur_phys, cur_core) != 0) {
          hwb_disable_pair_tracking(&pairs, &pair_count, &pair_cap, &can_track_pairs);
        }
      }
      cur_phys = -1;
      cur_core = -1;
      logical++;
    } else if (strncmp(line, "physical id", 11) == 0) {
      char* p = strchr(line, ':');
      if (p) cur_phys = atoi(p + 1);
    } else if (strncmp(line, "core id", 7) == 0) {
      char* p = strchr(line, ':');
      if (p) cur_core = atoi(p + 1);
    }
  }

  if (can_track_pairs && cur_phys >= 0 && cur_core >= 0) {
    if (hwb_add_pair(&pairs, &pair_count, &pair_cap, cur_phys, cur_core) != 0) {
      hwb_disable_pair_tracking(&pairs, &pair_count, &pair_cap, &can_track_pairs);
    }
  }

  *logical_cores = logical;
  *physical_cores = (can_track_pairs && pair_count > 0) ? pair_count : 0;
  free(pairs);
  return 0;
}

static void detect_linux_cpu(hwb_hardware_info* out) {
  FILE* f = fopen("/proc/cpuinfo", "r");
  if (!f) return;

  int logical = 0;
  int physical = 0;
  if (hwb_parse_linux_cpuinfo_stream(f, out->cpu_model, sizeof(out->cpu_model), &logical, &physical) == 0) {
    if (logical > 0) out->logical_cores = logical;
    if (physical > 0) out->physical_cores = physical;
  }
  fclose(f);
}

static void detect_linux_memory(hwb_hardware_info* out) {
  FILE* f = fopen("/proc/meminfo", "r");
  if (!f) return;

  char line[256];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "MemTotal:", 9) == 0) {
      unsigned long long kb = 0;
      if (sscanf(line, "MemTotal: %llu kB", &kb) == 1) {
        out->memory_total_mb = kb / 1024ULL;
      }
      break;
    }
  }
  fclose(f);
}

static void detect_linux_storage(hwb_hardware_info* out) {
  struct statvfs vfs;
  if (statvfs("/", &vfs) == 0) {
    unsigned long long total = (unsigned long long)vfs.f_frsize * (unsigned long long)vfs.f_blocks;
    out->storage_total_gb = total / (1024ULL * 1024ULL * 1024ULL);
  }

  DIR* dir = opendir("/sys/block");
  if (!dir) return;

  struct dirent* ent;
  while ((ent = readdir(dir)) != NULL) {
    const char* dev = ent->d_name;
    if (dev[0] == '.') continue;
    if (strncmp(dev, "loop", 4) == 0 || strncmp(dev, "ram", 3) == 0 ||
        strncmp(dev, "dm-", 3) == 0 || strncmp(dev, "md", 2) == 0) {
      continue;
    }

    char model_path[PATH_MAX];
    snprintf(model_path, sizeof(model_path), "/sys/block/%s/device/model", dev);
    FILE* m = fopen(model_path, "r");
    char model[HWB_HWSTR_MEDIUM] = {0};
    if (m) {
      if (fgets(model, sizeof(model), m)) {
        hwb_trim_newline(model);
      }
      fclose(m);
    }

    if (model[0] == '\0') {
      char vendor_path[PATH_MAX];
      char vendor[HWB_HWSTR_SMALL] = {0};
      snprintf(vendor_path, sizeof(vendor_path), "/sys/block/%s/device/vendor", dev);
      FILE* v = fopen(vendor_path, "r");
      if (v) {
        if (fgets(vendor, sizeof(vendor), v)) hwb_trim_newline(vendor);
        fclose(v);
      }
      if (vendor[0] != '\0') {
        snprintf(model, sizeof(model), "%.63s %.63s", vendor, dev);
      } else {
        hwb_copy_string(model, sizeof(model), dev);
      }
    }

    if (out->storage_name[0] == '\0') {
      hwb_copy_string(out->storage_name, sizeof(out->storage_name), model);
    }
    hwb_append_csv_token(out->storage_devices, sizeof(out->storage_devices), model);
    out->storage_device_count += 1;
  }

  closedir(dir);
}

static void detect_linux_gpu(hwb_hardware_info* out) {
  char best[HWB_HWSTR_LARGE] = {0};

  for (int card = 0; card < 32; ++card) {
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/uevent", card);
    FILE* f = fopen(path, "r");
    if (!f) continue;

    char line[256];
    char driver[64] = {0};
    char pci[64] = {0};
    while (fgets(line, sizeof(line), f)) {
      if (strncmp(line, "DRIVER=", 7) == 0) {
        hwb_copy_string(driver, sizeof(driver), line + 7);
        hwb_trim_newline(driver);
      } else if (strncmp(line, "PCI_ID=", 7) == 0) {
        hwb_copy_string(pci, sizeof(pci), line + 7);
        hwb_trim_newline(pci);
      }
    }
    fclose(f);

    if (pci[0] || driver[0]) {
      char tmp[HWB_HWSTR_LARGE];
      snprintf(tmp, sizeof(tmp), "PCI %s%s%s", pci[0] ? pci : "unknown", driver[0] ? " (" : "", driver[0] ? driver : "");
      if (driver[0]) {
        size_t n = strlen(tmp);
        if (n + 1 < sizeof(tmp)) {
          tmp[n] = ')';
          tmp[n + 1] = '\0';
        }
      }

      if (!best[0]) {
        hwb_copy_string(best, sizeof(best), tmp);
      }

      snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/boot_vga", card);
      FILE* boot = fopen(path, "r");
      int is_boot = 0;
      if (boot) {
        int c = fgetc(boot);
        if (c == '1') is_boot = 1;
        fclose(boot);
      }

      if (is_boot) {
        hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), tmp);
        return;
      }
    }
  }

  if (best[0]) {
    hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), best);
  }
}
#endif

#if defined(__APPLE__)
static void detect_macos_cpu(hwb_hardware_info* out) {
  size_t sz = sizeof(out->cpu_model);
  if (sysctlbyname("machdep.cpu.brand_string", out->cpu_model, &sz, NULL, 0) != 0) {
    out->cpu_model[0] = '\0';
  }

  int logical = 0;
  int physical = 0;
  sz = sizeof(logical);
  if (sysctlbyname("hw.logicalcpu", &logical, &sz, NULL, 0) == 0) out->logical_cores = logical;
  sz = sizeof(physical);
  if (sysctlbyname("hw.physicalcpu", &physical, &sz, NULL, 0) == 0) out->physical_cores = physical;
}

static void detect_macos_memory_storage(hwb_hardware_info* out) {
  long long mem = 0;
  size_t sz = sizeof(mem);
  if (sysctlbyname("hw.memsize", &mem, &sz, NULL, 0) == 0 && mem > 0) {
    out->memory_total_mb = (unsigned long long)mem / (1024ULL * 1024ULL);
  }

  struct statfs fs;
  if (statfs("/", &fs) == 0) {
    unsigned long long total = (unsigned long long)fs.f_bsize * (unsigned long long)fs.f_blocks;
    out->storage_total_gb = total / (1024ULL * 1024ULL * 1024ULL);
  }
}

static void detect_macos_gpu(hwb_hardware_info* out) {
  int pipe_fds[2];
  if (pipe(pipe_fds) != 0) {
    return;
  }

  pid_t pid = fork();
  if (pid == 0) {
    /* child */
    close(pipe_fds[0]);
    dup2(pipe_fds[1], STDOUT_FILENO);
    close(pipe_fds[1]);
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }
    const char* cmd = "/usr/sbin/system_profiler";
    char* const args[] = { (char*)cmd, (char*)"SPDisplaysDataType", NULL };
    execv(cmd, args);
    _exit(127);
  } else if (pid < 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return;
  }

  close(pipe_fds[1]);
  FILE* f = fdopen(pipe_fds[0], "r");
  if (!f) {
    close(pipe_fds[0]);
    waitpid(pid, NULL, 0);
    return;
  }

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    char* tag = strstr(line, "Chipset Model:");
    if (tag) {
      tag += strlen("Chipset Model:");
      while (*tag == ' ' || *tag == '\t') ++tag;
      hwb_trim_newline(tag);
      hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), tag);
      break;
    }
  }
  fclose(f);
  waitpid(pid, NULL, 0);
}
#endif

#if defined(_WIN32)
static void detect_windows_cpu(hwb_hardware_info* out) {
  const char* ident = getenv("PROCESSOR_IDENTIFIER");
  if (ident && ident[0]) {
    hwb_copy_string(out->cpu_model, sizeof(out->cpu_model), ident);
  }

  /* Use all processor groups for an accurate logical core count on >64-CPU systems */
  DWORD logical = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if (logical == 0) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    logical = info.dwNumberOfProcessors;
  }
  out->logical_cores = (int)logical;

  /* Count physical cores; each RelationProcessorCore entry represents one core */
  out->physical_cores = 0;
  DWORD length = 0;
  BOOL got_size = GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &length);
  if (!got_size && GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buf =
        (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(length);
    if (buf) {
      if (GetLogicalProcessorInformationEx(RelationProcessorCore, buf, &length)) {
        int physical = 0;
        BYTE* ptr = (BYTE*)buf;
        BYTE* end = ptr + length;
        while (ptr < end) {
          SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* entry =
              (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)ptr;
          /* Guard against corrupt/truncated entries to prevent infinite loops
             or out-of-bounds reads */
          if (entry->Size < sizeof(*entry) ||
              (DWORD)(end - ptr) < entry->Size) {
            break;
          }
          if (entry->Relationship == RelationProcessorCore) {
            ++physical;
          }
          ptr += entry->Size;
        }
        out->physical_cores = physical;
      }
      free(buf);
    }
  }
  if (out->physical_cores < 1) {
    out->physical_cores = out->logical_cores;
  }
}

static void detect_windows_memory_storage(hwb_hardware_info* out) {
  MEMORYSTATUSEX mem;
  mem.dwLength = sizeof(mem);
  if (GlobalMemoryStatusEx(&mem)) {
    out->memory_total_mb = (unsigned long long)(mem.ullTotalPhys / (1024ULL * 1024ULL));
  }

  ULARGE_INTEGER free_bytes, total_bytes, total_free;
  if (GetDiskFreeSpaceExA("C:\\", &free_bytes, &total_bytes, &total_free)) {
    (void)free_bytes;
    (void)total_free;
    out->storage_total_gb = (unsigned long long)(total_bytes.QuadPart / (1024ULL * 1024ULL * 1024ULL));
  }
}

static void detect_windows_gpu(hwb_hardware_info* out) {
  DISPLAY_DEVICEA dd;
  ZeroMemory(&dd, sizeof(dd));
  dd.cb = sizeof(dd);

  for (DWORD i = 0; EnumDisplayDevicesA(NULL, i, &dd, 0); ++i) {
    if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
      hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), dd.DeviceString);
      return;
    }
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
  }

  ZeroMemory(&dd, sizeof(dd));
  dd.cb = sizeof(dd);
  if (EnumDisplayDevicesA(NULL, 0, &dd, 0)) {
    hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), dd.DeviceString);
  }
}
#endif

int hwb_detect_hardware(hwb_hardware_info* out) {
  if (!out) {
    return -1;
  }

  memset(out, 0, sizeof(*out));

#if defined(__linux__)
  detect_linux_cpu(out);
  detect_linux_memory(out);
  detect_linux_storage(out);
  detect_linux_gpu(out);
#elif defined(__APPLE__)
  detect_macos_cpu(out);
  detect_macos_memory_storage(out);
  detect_macos_gpu(out);
#elif defined(_WIN32)
  detect_windows_cpu(out);
  detect_windows_memory_storage(out);
  detect_windows_gpu(out);
#endif

  if (out->logical_cores <= 0) out->logical_cores = 1;
  if (out->physical_cores <= 0) out->physical_cores = out->logical_cores;
  if (out->cpu_model[0] == '\0') hwb_copy_string(out->cpu_model, sizeof(out->cpu_model), "unknown");
  if (out->storage_name[0] == '\0') hwb_copy_string(out->storage_name, sizeof(out->storage_name), "unknown");
  if (out->storage_devices[0] == '\0') {
    hwb_copy_string(out->storage_devices, sizeof(out->storage_devices), out->storage_name);
    if (out->storage_device_count == 0) {
      out->storage_device_count = 1;
    }
  }
  if (out->gpu_name[0] == '\0') hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), "unknown");

  return 0;
}
