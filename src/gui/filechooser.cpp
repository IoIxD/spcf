#include "gui.hpp"

static void MWAPI file_callback(MwWidget handle, void *user_data,
                                void *call_data) {
  GUI *self = (GUI *)user_data;

  MwDispatchUserHandler(handle, MwNcloseHandler, NULL);
  MwDestroyWidget(handle);
  GUI::ScanCreationEntry entry;
  entry.idx = 0;
  snprintf(entry.dir, sizeof(entry.dir), "%s", (char *)call_data);
  entry.labelName = std::string((char *)call_data);
  self->scanStartQueue.push_back(entry);

  if (self->device_window) {
    MwDestroyWidget(self->device_window);
  }
}

void GUI::file_choose_button_handler(MwWidget widget, void *user,
                                     void *client) {
  GUI *self = (GUI *)user;

  self->directory_chooser =
      MwDirectoryChooser(self->main_window, "Scan a Directory");
  MwAddUserHandler(self->directory_chooser, MwNdirectoryChosenHandler,
                   file_callback, self);
};
