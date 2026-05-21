#include "gui.hpp"
#include <algorithm>

static const std::string avail_file_exts[] = {
    ".apng", ".png",   ".avif", ".gif", ".jpg", ".jpeg",
    ".jfif", ".pjpeg", ".pjp",  ".png", ".svg", ".webp"};

static void MWAPI file_callback(MwWidget handle, void *user_data,
                                void *call_data) {
  GUI *self = (GUI *)user_data;
  memcpy(self->knownDir, call_data, 255);
  self->showingScan = true;

  self->activateScanner = 1;

  MwDispatchUserHandler(handle, MwNcloseHandler, NULL);
}

void GUI::scan_button_handler(MwWidget widget, void *user, void *client) {
  GUI *self = (GUI *)user;
  // self->milskoGUIMutex.lock();

  if (!self->model_context) {
    self->model_context = new ModelContext();
  }

  self->directory_chooser =
      MwDirectoryChooser(self->window, "Scan a Directory");
  MwAddUserHandler(self->directory_chooser, MwNdirectoryChosenHandler,
                   file_callback, self);
  // self->milskoGUIMutex.unlock();
};

void GUI::window_scan_thing(MwWidget widget, void *user, void *client) {
  GUI *self = (GUI *)user;
  if (self->activateScanner == 1) {
    self->activateScanner = 2;
  } else if (self->activateScanner == 2) {
    self->scan_boxes = MwTabAdd(self->tab_view, self->knownDir);

    MwStep(self->tab_view);

    MwStep(self->scan_boxes);

    self->activateScanner = 3;
  } else if (self->activateScanner == 3) {
    int width = MwGetInteger(self->scan_boxes, MwNwidth);
    int height = MwGetInteger(self->scan_boxes, MwNheight);
    MwWidget box = MwVaCreateWidget(MwTreeViewClass, "box", self->scan_boxes, 0,
                                    0, width - 1, height - 1, NULL);
    self->dir_recurse(self->knownDir, [=](std::filesystem::path path) {
      auto e = path.extension().string();
      std::transform(e.begin(), e.end(), e.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      for (auto ext : avail_file_exts) {
        if (e == ext) {
          self->model_context->scan(path.string().c_str());
          std::string foundLabels = "";

          for (int i = 0; i < 32; i++) {
            char name[255] = {0};
            self->model_context->get_scanned_name(i, name);
            foundLabels += name;
            foundLabels += ", ";
          }

          char f[255];
          snprintf(f, 255, "%s - %s", path.filename().c_str(),
                   foundLabels.c_str());
          MwTreeViewAdd(box, NULL, NULL, f);
          MwStep(box);
          break;
        };
      }
    });
    self->scans.push_back(
        (GUI::ScanEntry){.box = box, .col1 = NULL, .col2 = NULL});
    self->activateScanner = 0;
  }
}
