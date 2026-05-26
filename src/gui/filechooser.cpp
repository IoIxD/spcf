#include "gui.hpp"

static void MWAPI file_callback(MwWidget handle, void *user_data,
                                void *call_data) {
  GUI *self = (GUI *)user_data;

  /* We call this function outside the context of Milsko (in the dbus
   * functions), and when we do this, handle is null since there's no file
   * chooser widget to destroy
   */
  if (handle) {
    MwDispatchUserHandler(handle, MwNcloseHandler, NULL);
    MwDestroyWidget(handle);
  }

  GUI::ScanCreationEntry entry;
  entry.idx = 0;
  snprintf(entry.dir, sizeof(entry.dir), "%s", (char *)call_data);
  entry.labelName = std::string((char *)call_data);
  self->scanStartQueue.push_back(entry);

  if (self->device_window) {
    MwDestroyWidget(self->device_window);
  }
}
#ifdef __linux__
static void dbus_tick(MwWidget handle, void *user_data, void *call_data) {
  GUI::DBusContext *dbus = (GUI::DBusContext *)user_data;
  if (dbus->status() != -1) {
    return;
  }
  dbus_connection_read_write_dispatch(dbus->conn(), 50);
}

static void onError(std::string err, void *ud) {
  GUI *self = (GUI *)ud;
  self->errMutex.lock();
  self->errorCreationQueue.push_back(err);
  self->errMutex.unlock();
}

void GUI::start_dbus_filechooser(MwUserHandler handler) {
  dbus.open_file(this, handler, onError, this);
  MwAddUserHandler(main_window->main_window, MwNtickHandler, dbus_tick, &dbus);
};
#endif

void GUI::file_choose_button_handler(MwWidget widget, void *user,
                                     void *client) {
  GUI *self = (GUI *)user;

#ifdef __linux__
  if (self->dbus.valid()) {
    self->start_dbus_filechooser(file_callback);
  } else
#endif
  {
    self->main_window->directory_chooser =
        MwDirectoryChooser(self->main_window->main_window, "Scan a Directory");
    MwAddUserHandler(self->main_window->directory_chooser,
                     MwNdirectoryChosenHandler, file_callback, self);
  }
};
