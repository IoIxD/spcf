#pragma once
#include "../model/model.hpp"
#include <Mw/Milsko.h>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>

class GUI {

public:
  MwWidget window = NULL;
  MwWidget main_box = NULL;

  MwWidget tab_view = NULL;
  MwWidget search_results_box = NULL;

  MwWidget search_box_holder = NULL;
  MwWidget search_box_text = NULL;
  MwWidget search_box = NULL;

  MwWidget scan_button_holder = NULL;
  MwWidget scan_button = NULL;
  MwWidget directory_chooser = NULL;

  ModelContext *model_context = NULL;

  bool showingScan = false;

  std::mutex scanMutex;
  std::unordered_map<std::filesystem::path, char> scannedFiles;
  struct ScanCreationEntry {
    int idx;
    char dir[255];
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

  int activateScanner = 0;

  GUI();

  void dir_recurse(const std::string &path,
                   std::function<void(const std::filesystem::path &)> cb);

  static void scan_button_handler(MwWidget widget, void *user, void *client);
  static void window_scan_thing(MwWidget widget, void *user, void *client);
};
