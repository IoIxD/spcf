#include "gui.hpp"
#include <algorithm>
void GUI::dir_recurse(const std::string &path,
                      std::function<void(const std::filesystem::path &)> cb) {
  if (auto dir = opendir(path.c_str())) {
    while (auto f = readdir(dir)) {
      if (f->d_name[0] == '.')
        continue;
      struct stat buf;
      lstat((path + f->d_name).c_str(), &buf);
      if (S_ISDIR(buf.st_mode)) {
        dir_recurse(path + std::filesystem::path::preferred_separator +
                        f->d_name + std::filesystem::path::preferred_separator,
                    cb);
      } else {
        cb(path + f->d_name);
      }
    }
    closedir(dir);
  }
};

GUI::GUI() {
  main_window = MwVaCreateWidget(MwWindowClass, "main", NULL, MwDEFAULT,
                                 MwDEFAULT, 640, 480, MwNtitle, "SPCF", NULL);

  main_box = MwVaCreateWidget(MwBoxClass, "box", main_window, 0, 0, 640, 480,
                              MwNorientation, MwVERTICAL, MwNpadding, 16, NULL);

  search_box_holder = MwVaCreateWidget(MwBoxClass, "entry_box", main_box, 0, 0,
                                       640, 16, MwNratio, 1, NULL);

  device_scan_button_holder = MwVaCreateWidget(
      MwBoxClass, "entry_box", main_box, 0, 0, 640, 16, MwNratio, 1, NULL);
  device_scan_button =
      MwVaCreateWidget(MwButtonClass, "entry", device_scan_button_holder, 0, 0,
                       640, 16, MwNratio, 8, MwNtext, "Scan", NULL);

  search_box_text =
      MwVaCreateWidget(MwLabelClass, "entry_text", search_box_holder, 0, 0, 1,
                       1, MwNtext, "Search", MwNratio, 1, NULL);
  search_box = MwVaCreateWidget(MwEntryClass, "entry", search_box_holder, 0, 0,
                                640, 16, MwNratio, 8, NULL);
  tab_view = MwVaCreateWidget(MwTabClass, "entry", main_box, 0, 0, 640, 16,
                              MwNratio, 16, NULL);
  search_results_box = MwTabAdd(tab_view, "Results");

  MwAddUserHandler(device_scan_button, MwNactivateHandler,
                   device_choose_button_handler, this);
  MwAddUserHandler(main_window, MwNtickHandler, window_tick, this);
}

void GUI::start_scan(std::string dir) {
  if (!modelContext) {
    modelContext = new ModelContext();
  }
  GUI::ScanCreationEntry creationEntry;
  this->showingScan = true;

  int i = this->scanBoxEntryQueue.size();
  creationEntry.idx = i;
  memcpy(creationEntry.dir, dir.c_str(), 255);

  this->scanBoxCreationQueue.push_back(creationEntry);

  printf("starting scan of %s\n", dir.c_str());

  this->scanThreads.push_back(new std::thread([=]() {
    this->dir_recurse(creationEntry.dir, [=](std::filesystem::path path) {
      auto e = path.extension().string();
      std::transform(e.begin(), e.end(), e.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      for (auto ext : avail_file_exts) {
        if (e == ext) {
          this->modelContext->scan(path.string().c_str());
          std::string foundLabels = "";

          for (int i = 0; i < 32; i++) {
            char name[255] = {0};
            this->modelContext->get_scanned_name(i, name);
            foundLabels += name;
            foundLabels += ", ";
          }
          GUI::ScanEntry entry = {
              .idx = i,
          };
          snprintf(entry.line1, 255, "%s", path.filename().c_str());
          snprintf(entry.line2, 255, "%s", foundLabels.c_str());

          this->tickMutex.lock();
          this->scanBoxEntryQueue.push_back(entry);
          this->tickMutex.unlock();

          break;
        };
      }
    });
  }));
  this->activateScanner = 1;
}

static void MWAPI ok(MwWidget handle, void *user, void *call) {
  (void)handle;
  (void)call;
  MwDestroyWidget(MwGetParent(handle));
}

void GUI::window_tick(MwWidget widget, void *user, void *client) {
  GUI *self = (GUI *)user;
  self->tickMutex.lock();

  bool didErrorCreate = false;
  bool didSBCCreate = false;
  bool didSBECreate = false;

  for (auto err : self->errorCreationQueue) {
    MwWidget mb = MwMessageBox(widget, err.c_str(), "Error",
                               MwMB_ICONERROR | MwMB_BUTTONOK);
    MwAddUserHandler(MwMessageBoxGetChild(mb, MwMB_BUTTONOK),
                     MwNactivateHandler, ok, mb);
    MwReparent(mb, self->main_window);
    didErrorCreate = true;
  }
  if (didErrorCreate)
    self->errorCreationQueue.erase(self->errorCreationQueue.begin());

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
    didSBCCreate = true;
  }
  if (didSBCCreate)
    self->scanBoxCreationQueue.erase(self->scanBoxCreationQueue.begin());

  for (auto sc : self->scanBoxEntryQueue) {
    int index = MwListBoxSet(self->scanLines[sc.idx].box, -1, 0, sc.line1);
    MwListBoxSet(self->scanLines[sc.idx].box, index, -1, sc.line2);
    didSBECreate = true;
  }
  if (didSBECreate)
    self->scanBoxEntryQueue.erase(self->scanBoxEntryQueue.begin());
  self->tickMutex.unlock();
}
