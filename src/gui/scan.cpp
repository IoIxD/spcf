#include "../image/image.hpp"
#include "gui.hpp"
#include <algorithm>

void GUI::dir_recurse(const std::string &path,
                      std::function<void(const std::filesystem::path &)> cb) {
  if (auto dir = opendir(path.c_str())) {
    while (auto f = readdir(dir)) {
      if (f->d_name[0] == '.')
        continue;
      struct stat buf;
      lstat((path + std::filesystem::path::preferred_separator + f->d_name)
                .c_str(),
            &buf);
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
static void onError(std::string err, void *ud) {
  GUI *self = (GUI *)ud;
  self->errMutex.lock();
  self->errorCreationQueue.push_back(err);
  self->errMutex.unlock();
}

void GUI::start_scan(std::string dir, std::string tblName) {
  if (!modelContext) {
    modelContext = new ModelContext();
    if (!modelContext->valid()) {
      onError(modelContext->error(), this);
      return;
    }
  }
  GUI::ScanCreationEntry creationEntry;
  this->showingScan = true;

  int i = this->scanBoxEntryQueue.size();
  creationEntry.idx = i;
  memcpy(creationEntry.dir, dir.c_str(), 255);

  this->scanBoxCreationQueue.push_back(creationEntry);

  printf("starting scan of %s\n", dir.c_str());

  if (!mDB.create_table(tblName, onError, this)) {
    return;
  };

  this->scanThreads.push_back(new std::thread([=]() {
    this->dir_recurse(creationEntry.dir, [=](std::filesystem::path path) {
      auto e = path.extension().string();
      std::transform(e.begin(), e.end(), e.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      for (auto ext : avail_file_exts) {
        if (e == ext) {
          std::string p = path.string();
          if (path.string().find(creationEntry.dir) != std::string::npos) {
            p = p.substr(strlen(creationEntry.dir));
          }

          mPool.submit_task([=]() {
            stdoutMutex.lock();
            printf(">%s\n", p.c_str());
            stdoutMutex.unlock();

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
            snprintf(entry.line1, 255, "%s", p.c_str());
            snprintf(entry.line2, 255, "%s", foundLabels.c_str());

            // Image *img = image_get(path.string().c_str());

            // uint8_t *data = image_get_pixels(img);
            // size_t len = image_get_pixel_len(img);

            mDB.new_entry(tblName, entry.line1, entry.line2, onError, this);

            // image_free(img);

            this->tickMutex.lock();
            this->scanBoxEntryQueue.push_back(entry);
            this->tickMutex.unlock();
          });

          break;
        };
      }
    });
  }));
}
