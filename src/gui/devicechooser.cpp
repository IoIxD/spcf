#include "../admin/admin.hpp"
#include "../devices/devices.hpp"
#include "gui.hpp"

static void mount_success(void *user, std::string mountpoint) {
  GUI *self = (GUI *)user;
  printf("success. now scanning %s\n", mountpoint.c_str());
  GUI::ScanCreationEntry entry;
  entry.idx = 0;
  snprintf(entry.dir, sizeof(entry.dir), "%s", mountpoint.c_str());
  entry.labelName = self->scanDeviceName;
  self->scanStartQueue.push_back(entry);
}
static void mount_err(void *user, std::string err) {
  GUI *self = (GUI *)user;
  self->errMutex.lock();
  self->errorCreationQueue.push_back(err);
  self->errMutex.unlock();
}

static void device_activate(MwWidget handle, void *user, void *call) {
  GUI *self = (GUI *)user;
  char msg[256];
  MwWidget msgbox;

  auto name = MwListBoxGet(handle, *(int *)call);
  for (auto dev : self->scannedDevices) {
    if (dev.label() == std::string(name)) {
      printf("Mounting %s\n", dev.label().c_str());

      self->scanDeviceName = dev.label();
      dev.mount(mount_success, mount_err, user);

      MwDestroyWidget(self->device_window);
    }
  }
}

void GUI::device_choose_button_handler(MwWidget widget, void *user,
                                       void *client) {
  GUI *self = (GUI *)user;
  self->device_window =
      MwVaCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT, 320,
                       240, MwNtitle, "Select Device", NULL);

  MwReparent(self->device_window, self->main_window);

  auto admin = is_admin();
  if (!admin) {
    self->device_warning =
        MwVaCreateWidget(MwLabelClass, "warning", self->device_window, 15, 15,
                         320 - 32, 64, MwNtext,
                         "You must"
#ifdef _WIN32
                         "be an administrator\nto scan/mount"
#else
                         " run as root to scan/mount\n"
#endif
                         "external devices."
#ifdef __linux__
                         " (or install polkit)\n",
#endif

                         MwNforeground, "red", NULL);
    /* TODO: On platforms like Windows where we can request administrator right
     * here, have a button for doing so. */
  }
  self->device_listbox = MwVaCreateWidget(
      MwListBoxClass, "listbox", self->device_window, 15, admin ? 15 : 64 + 15,
      320 - 32, 240 - (admin ? 64 : 128), MwNdisabled, !admin, NULL);
  if (admin) {
    self->scannedDevices = Device::get();
    for (auto dev : self->scannedDevices) {
      if (dev.type() == Device::Type::Unknown) {
        continue;
      }
      printf("found %s\n", dev.label().c_str());
      MwListBoxSet(self->device_listbox, -1, 0, dev.label().c_str());
      MwAddUserHandler(self->device_listbox, MwNlistBoxActivateHandler,
                       device_activate, self);
    }
  }

  self->device_window_browse =
      MwVaCreateWidget(MwButtonClass, "b", self->device_window, 320 - 15 - 75,
                       240 - 15 - 25, 75, 25, MwNtext, "Browse", NULL);
  MwAddUserHandler(self->device_window_browse, MwNactivateHandler,
                   file_choose_button_handler, self);
};
