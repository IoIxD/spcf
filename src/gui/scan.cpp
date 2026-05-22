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
