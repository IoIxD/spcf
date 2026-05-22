
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
bool no_pkexec = false;

bool can_request_admin() { return false; };
void request_admin() { /* no-op */ };
bool is_admin() {
  /* If we have polkit, return that we are admin even though we aren't. The code
   * that then uses this function just uses pkexec to execute admin tasks. */
  return polkit_path() || (getuid() == 0);
};

const char *polkit_path() {
  if (no_pkexec) {
    return NULL;
  }
  if (!found_pkexec) {
    char *dup = strdup(getenv("PATH"));
    char *s = dup;
    char *p = NULL;
    memset(g_pkexec_path, 0, sizeof(g_pkexec_path));

    do {
      p = strchr(s, ':');
      if (p != NULL) {
        p[0] = 0;
      }
      snprintf(g_pkexec_path, PATH_MAX, "%s/pkexec", s);
      if (std::filesystem::exists(g_pkexec_path)) {
        found_pkexec = true;
        free(dup);
        goto ret;
      }
      s = p + 1;
    } while (p != NULL);

    free(dup);
    no_pkexec = true;
    return NULL;
  }
ret:
  return g_pkexec_path;
}

#endif
