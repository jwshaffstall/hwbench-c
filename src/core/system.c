#include "hwbench/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <sys/mount.h>
  #include <sys/sysctl.h>
#else
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
static void detect_linux_cpu(hwb_hardware_info* out) {
  FILE* f = fopen("/proc/cpuinfo", "r");
  if (!f) return;

  char line[512];
  int logical = 0;
  int physical = 0;
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "model name", 10) == 0 && out->cpu_model[0] == '\0') {
      char* p = strchr(line, ':');
      if (p) {
        p += 1;
        while (*p == ' ' || *p == '\t') ++p;
        hwb_trim_newline(p);
        hwb_copy_string(out->cpu_model, sizeof(out->cpu_model), p);
      }
    } else if (strncmp(line, "processor", 9) == 0) {
      logical++;
    } else if (strncmp(line, "cpu cores", 9) == 0 && physical == 0) {
      char* p = strchr(line, ':');
      if (p) {
        physical = atoi(p + 1);
      }
    }
  }
  fclose(f);

  out->logical_cores = logical > 0 ? logical : out->logical_cores;
  if (physical > 0) {
    out->physical_cores = physical;
  }
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

  FILE* m = fopen("/sys/block/nvme0n1/device/model", "r");
  if (!m) m = fopen("/sys/block/sda/device/model", "r");
  if (!m) m = fopen("/sys/block/vda/device/model", "r");
  if (m) {
    char buf[HWB_HWSTR_MEDIUM] = {0};
    if (fgets(buf, sizeof(buf), m)) {
      hwb_trim_newline(buf);
      hwb_copy_string(out->storage_name, sizeof(out->storage_name), buf);
    }
    fclose(m);
  }
}

static void detect_linux_gpu(hwb_hardware_info* out) {
  FILE* f = fopen("/sys/class/drm/card0/device/uevent", "r");
  if (f) {
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
      hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), tmp);
      return;
    }
  }

  f = popen("lspci 2>/dev/null", "r");
  if (!f) return;
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strstr(line, "VGA compatible controller") || strstr(line, "3D controller")) {
      hwb_trim_newline(line);
      hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), line);
      break;
    }
  }
  pclose(f);
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
  FILE* f = popen("system_profiler SPDisplaysDataType 2>/dev/null", "r");
  if (!f) return;
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
  pclose(f);
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
  FILE* f = _popen("wmic path win32_VideoController get Name /value", "r");
  if (!f) return;

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "Name=", 5) == 0) {
      char* name = line + 5;
      hwb_trim_newline(name);
      if (name[0]) {
        hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), name);
        break;
      }
    }
  }
  _pclose(f);
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
  if (out->gpu_name[0] == '\0') hwb_copy_string(out->gpu_name, sizeof(out->gpu_name), "unknown");

  return 0;
}
