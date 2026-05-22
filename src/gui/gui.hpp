#pragma once
#include "../database/database.hpp"
#include "../devices/devices.hpp"
#include "../model/model.hpp"
#include <Mw/Milsko.h>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>

class GUI {
  Database mDB;

public:
  MwWidget main_window = NULL;
  MwWidget main_box = NULL;
  MwWidget tab_view = NULL;
  MwWidget search_results_box = NULL;
  MwWidget search_box_holder = NULL;
  MwWidget search_box_text = NULL;
  MwWidget search_box = NULL;
  MwWidget search_box_button = NULL;
  MwWidget device_scan_button_holder = NULL;
  MwWidget device_scan_button = NULL;
  MwWidget directory_chooser = NULL;
  MwWidget scan_button_holder = NULL;
  MwWidget scan_button = NULL;

  MwWidget device_window = NULL;
  MwWidget device_list = NULL;
  MwWidget device_warning = NULL;
  MwWidget device_listbox = NULL;
  MwWidget device_window_browse = NULL;

  bool showingScan = false;
  ModelContext *modelContext = NULL;

  std::mutex tickMutex;
  std::mutex errMutex;
  std::unordered_map<std::filesystem::path, char> scannedFiles;
  struct ScanCreationEntry {
    int idx;
    char dir[255];
    std::string labelName;
  };
  struct ScanLine {
    MwWidget box = NULL;
    MwWidget tab = NULL;
    int count = 0;
  };
  struct ScanEntry {
    int idx = 0;
    char line1[255];
    char line2[255];
  };
  std::vector<ScanCreationEntry> scanBoxCreationQueue;
  std::vector<ScanEntry> scanBoxEntryQueue;
  std::vector<ScanLine> scanLines;

  std::vector<std::thread *> scanThreads;
  std::vector<std::thread *> scanStarterThreads;
  std::vector<Device> scannedDevices;
  std::vector<std::string> errorCreationQueue;
  std::vector<ScanCreationEntry> scanStartQueue;
  std::string scanDeviceName;

  GUI();

  void dir_recurse(const std::string &path,
                   std::function<void(const std::filesystem::path &)> cb);

  void start_scan(std::string dir, std::string tbl);

  static void file_choose_button_handler(MwWidget widget, void *user,
                                         void *client);
  static void device_choose_button_handler(MwWidget widget, void *user,
                                           void *client);
  static void window_tick(MwWidget widget, void *user, void *client);
};

static const std::string avail_file_exts[] = {
    ".apng", ".png",   ".avif", ".gif", ".jpg", ".jpeg",
    ".jfif", ".pjpeg", ".pjp",  ".png", ".svg", ".webp"};
