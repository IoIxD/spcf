#pragma once

#include <linux/limits.h>
#include <string>
#include <vector>

class Device {
public:
  enum class Type { CD = 0, Unknown };

private:
  char mLabel[PATH_MAX + 1];
  Type mType;

#ifdef __linux__
  char mDevName[PATH_MAX + 1];
  char mUUID[PATH_MAX + 1];
  char mFSType[255];
#endif

public:
  /**
   * Scan for any devices, mounted or not, that could be scanned.
   */
  static std::vector<Device> get();

  /**
   * Mount the selected device. Calls successFunc on success, errFunc otherwise.
   */
  void mount(void (*successFunc)(void *, std::string),
             void (*errFunc)(void *, std::string), void *ud);
  /** Unmount the selected device. */
  void unmount();

  std::string label() { return mLabel; };
  Device::Type type() { return mType; }
};
