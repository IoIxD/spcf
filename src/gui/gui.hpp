#pragma once
#include "../model/model.hpp"
#include <Mw/Milsko.h>
#include <filesystem>
#include <functional>

class GUI {

public:
  char knownDir[255];
  MwWidget window = NULL;
  MwWidget main_box = NULL;

  MwWidget tab_view = NULL;
  MwWidget search_results_box = NULL;
  MwWidget scan_boxes;

  MwWidget search_box_holder = NULL;
  MwWidget search_box_text = NULL;
  MwWidget search_box = NULL;

  MwWidget scan_button_holder = NULL;
  MwWidget scan_button = NULL;
  MwWidget directory_chooser = NULL;

  ModelContext *model_context = NULL;

  bool showingScan = false;

  struct ScanEntry {
    MwWidget box;
    MwWidget col1;
    MwWidget col2;
  };
  std::vector<ScanEntry> scans;
  int activateScanner = 0;
  // std::mutex milskoGUIMutex;

  GUI();

  void dir_recurse(const std::string &path,
                   std::function<void(const std::filesystem::path &)> cb);

  static void scan_button_handler(MwWidget widget, void *user, void *client);
  static void window_scan_thing(MwWidget widget, void *user, void *client);
};
