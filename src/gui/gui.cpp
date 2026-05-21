#include "gui.hpp"
void GUI::dir_recurse(const std::string &path,
                      std::function<void(const std::filesystem::path &)> cb) {
  if (auto dir = opendir(path.c_str())) {
    while (auto f = readdir(dir)) {
      if (f->d_name[0] == '.')
        continue;
      if (f->d_type == DT_DIR) {
        dir_recurse(path + std::filesystem::path::preferred_separator +
                        f->d_name + std::filesystem::path::preferred_separator,
                    cb);
      } else if (f->d_type == DT_REG) {
        cb(path + f->d_name);
      }
    }
    closedir(dir);
  }
};

GUI::GUI() {
  window = MwCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT,
                          640, 480);

  main_box = MwVaCreateWidget(MwBoxClass, "box", window, 0, 0, 640, 480,
                              MwNorientation, MwVERTICAL, MwNpadding, 16, NULL);

  search_box_holder = MwVaCreateWidget(MwBoxClass, "entry_box", main_box, 0, 0,
                                       640, 16, MwNratio, 1, NULL);
  scan_button_holder = MwVaCreateWidget(MwBoxClass, "entry_box", main_box, 0, 0,
                                        640, 16, MwNratio, 1, NULL);

  search_box_text =
      MwVaCreateWidget(MwLabelClass, "entry_text", search_box_holder, 0, 0, 1,
                       1, MwNtext, "Search", MwNratio, 1, NULL);
  search_box = MwVaCreateWidget(MwEntryClass, "entry", search_box_holder, 0, 0,
                                640, 16, MwNratio, 8, NULL);
  tab_view = MwVaCreateWidget(MwTabClass, "entry", main_box, 0, 0, 640, 16,
                              MwNratio, 16, NULL);
  search_results_box = MwTabAdd(tab_view, "Results");

  scan_button =
      MwVaCreateWidget(MwButtonClass, "entry", scan_button_holder, 0, 0, 640,
                       16, MwNratio, 8, MwNtext, "Scan Directory", NULL);
  MwAddUserHandler(scan_button, MwNactivateHandler, scan_button_handler, this);
  MwAddUserHandler(window, MwNtickHandler, window_scan_thing, this);
}
