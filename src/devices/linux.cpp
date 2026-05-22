#ifdef __linux__
#include <unistd.h>

#include "../admin/admin.hpp"
#include "devices.hpp"
#include <blkid/blkid.h>
#include <filesystem>
#include <linux/limits.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>

std::vector<Device> Device::get() {
  std::vector<Device> devs;
  blkid_cache cache;

  blkid_get_cache(&cache, NULL);
  blkid_probe_all_removable(cache);

  auto iter = blkid_dev_iterate_begin(cache);
  blkid_dev dev;
  while (blkid_dev_next(iter, &dev) == 0) {
    Device d;
    memset(d.mLabel, 0, 255);
    memset(d.mDevName, 0, 255);
    memset(d.mFSType, 0, 255);
    d.mType = Type::Unknown;

    memcpy(d.mDevName, blkid_dev_devname(dev), PATH_MAX);
    memcpy(d.mUUID, blkid_dev_devname(dev), PATH_MAX);

    blkid_probe probe = blkid_new_probe_from_filename(d.mDevName);

    if (probe) {
      if (blkid_do_probe(probe) == 0) {
        const char *name;
        const char *data;
        size_t len;
        int nvals = blkid_probe_numof_values(probe);
        for (int n = 0; n < nvals; n++) {
          if (blkid_probe_get_value(probe, n, &name, &data, &len) == 0) {
            if (strcmp(name, "LABEL") == 0) {
              memcpy(d.mLabel, data, sizeof(d.mLabel) - 1);
            } else if (strcmp(name, "UUID") == 0) {
              memcpy(d.mUUID, data, strlen(data));
            } else if (strcmp(name, "TYPE") == 0) {
              memcpy(d.mFSType, data, strlen(data));
              if (strcmp(data, "iso9660") == 0) {
                d.mType = Type::CD;
              } else {
                printf("WARNING: %s has unknown type \"%s\"\n", d.mDevName,
                       d.mLabel);
              }
            }
          }
        }
      }
    }

    blkid_free_probe(probe);

    if (strcmp(d.mLabel, "") != 0) {
      devs.push_back(d);
    }
  }

  blkid_dev_iterate_end(iter);

  blkid_put_cache(cache);

  return devs;
};

void Device::mount(void (*successFunc)(void *, std::string),
                   void (*errFunc)(void *, std::string), void *ud) {
  char error[256];
  std::filesystem::path path = mDevName;
  printf("mounting %s (%s)\n", mDevName, mFSType);
  char mountpoint[PATH_MAX];
  memset(mountpoint, 0, PATH_MAX);
  char cwd[PATH_MAX];
  memset(cwd, 0, PATH_MAX);
  getcwd(cwd, PATH_MAX);
  snprintf(mountpoint, PATH_MAX, "%s/.spcf/%s/", cwd, mLabel);
  if (!std::filesystem::exists(mountpoint)) {
    std::filesystem::create_directories(mountpoint);
  }
  auto err = ::mount(mDevName, mountpoint, mFSType,
                     MS_RDONLY | MS_NOEXEC | MS_NOATIME, NULL);
  if (err != 0) {
    /* try and use pkexec if permission was denied. */
    if (errno == EPERM && polkit_path()) {
      int f;
      int pipefd[2];
      pipe(pipefd);
      if ((f = fork()) == 0) {
        close(pipefd[0]); // close reading end in the child

        dup2(pipefd[1], 2); // send stderr to the pipe

        close(pipefd[1]); // this descriptor is no longer needed

        char *args[] = {(char *)"pkexec", (char *)"mount", mDevName, mountpoint,
                        NULL};
        printf("running(?) pkexec mount %s %s\n", mDevName, mountpoint);
        execve(polkit_path(), args, NULL);
        exit(0);
      } else {
        waitpid(f, NULL, 0);

        char buffer[4096];
        char *buffer_ptr = buffer;

        close(pipefd[1]); // close the write end of the pipe in the parent

        while (read(pipefd[0], buffer, sizeof(buffer)) != 0) {
        }

        if (strlen(buffer_ptr) != 0) {
          char *point;
          while ((point = strstr(buffer_ptr, ": "))) {
            buffer_ptr[point - buffer + 1] = '\n';
          }
          if (strstr(buffer_ptr,
                     "source write-protected, mounted read-only.")) {
            successFunc(ud, mountpoint);
          } else {
            errFunc(ud, buffer_ptr);
          }
        } else {
          printf("no error. calling success func\n");
          successFunc(ud, mountpoint);
        }
        return;
      }
    } else {
      snprintf(error, 255, "%s (Error %d)", strerror(errno), errno);
      errFunc(ud, error);
    }
  }
  successFunc(ud, mountpoint);
}

#endif
