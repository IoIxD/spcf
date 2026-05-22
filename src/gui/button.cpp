#include "gui.hpp"
#include <algorithm>

static const std::string avail_file_exts[] = {
    ".apng", ".png",   ".avif", ".gif", ".jpg", ".jpeg",
    ".jfif", ".pjpeg", ".pjp",  ".png", ".svg", ".webp"};

static void MWAPI file_callback(MwWidget handle, void *user_data,
                                void *call_data) {
  GUI *self = (GUI *)user_data;
  GUI::ScanCreationEntry creationEntry;
  self->showingScan = true;

  int i = self->scanBoxEntryQueue.size();
  creationEntry.idx = i;
  memcpy(creationEntry.dir, call_data, 255);

  self->scanBoxCreationQueue.push_back(creationEntry);

  self->scanThreads.push_back(new std::thread([=]() {
    self->dir_recurse(creationEntry.dir, [=](std::filesystem::path path) {
      self->scanMutex.lock();
      if (self->scannedFiles.find(path) != self->scannedFiles.end()) {
        return;
      }
      self->scanMutex.unlock();

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
          GUI::ScanEntry entry = {
              .idx = i,
          };
          snprintf(entry.line1, 255, "%s", path.filename().c_str());
          snprintf(entry.line2, 255, "%s", foundLabels.c_str());

          self->scanMutex.lock();
          self->scanBoxEntryQueue.push_back(entry);
          self->scanMutex.unlock();

          break;
        };
      }

      self->scannedFiles.insert_or_assign(path, 0);
    });
  }));
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
  bool didScanBoxCreate = false;
  self->scanMutex.lock();
  for (auto sc : self->scanBoxCreationQueue) {
    if (sc.idx >= self->scanLines.size()) {
      self->scanLines.resize(sc.idx + 1);
    }
    self->scanLines[sc.idx].tab = MwTabAdd(self->tab_view, sc.dir);

    int width = MwGetInteger(self->scanLines[sc.idx].tab, MwNwidth);
    int height = MwGetInteger(self->scanLines[sc.idx].tab, MwNheight);
    MwWidget box =
        MwVaCreateWidget(MwListBoxClass, "box", self->scanLines[sc.idx].tab, 0,
                         0, width - 1, height - 1, NULL);
    MwListBoxSetWidth(box, 0, -384);
    int index = MwListBoxSet(box, -1, 0, "Filename");
    MwListBoxSet(box, index, -1, "Keywords");

    self->scanLines[sc.idx].box = box;

    didScanBoxCreate = true;
  }
  if (didScanBoxCreate) {
    self->scanBoxCreationQueue.erase(self->scanBoxCreationQueue.begin());
  }

  bool didScanEntryCreate = false;
  for (auto sc : self->scanBoxEntryQueue) {
    int index = MwListBoxSet(self->scanLines[sc.idx].box, -1, 0, sc.line1);
    MwListBoxSet(self->scanLines[sc.idx].box, index, -1, sc.line2);

    didScanEntryCreate = true;
  }
  if (didScanEntryCreate) {
    self->scanBoxEntryQueue.erase(self->scanBoxEntryQueue.begin());
  }
  self->scanMutex.unlock();
}
