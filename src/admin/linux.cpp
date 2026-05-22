
#ifdef __linux__
#include "admin.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

bool found_pkexec = false;
char g_pkexec_path[PATH_MAX + 1];

bool can_request_admin() { return false; };
void request_admin() {};
bool is_admin() { return polkit_path() || (getuid() == 0); };

const char *polkit_path() {
  if (!found_pkexec) {
    char *dup = strdup(getenv("PATH"));
    char *s = dup;
    char *p = NULL;
    do {
      p = strchr(s, ':');
      if (p != NULL) {
        p[0] = 0;
      }
      snprintf(g_pkexec_path, PATH_MAX, "%s/pkexec", s);
      if (std::filesystem::exists(g_pkexec_path)) {
        goto ret;
      }
      s = p + 1;
    } while (p != NULL);

    free(dup);
    found_pkexec = true;
  }
ret:
  return g_pkexec_path;
}

#endif
