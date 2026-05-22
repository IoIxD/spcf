#include "gui.hpp"
#include <algorithm>

static void MWAPI file_callback(MwWidget handle, void *user_data,
                                void *call_data) {
  GUI *self = (GUI *)user_data;

  self->activateScanner = 1;

  self->start_scan((char *)call_data);

  MwDispatchUserHandler(handle, MwNcloseHandler, NULL);
}

void GUI::file_choose_button_handler(MwWidget widget, void *user,
                                     void *client) {
  GUI *self = (GUI *)user;

  self->directory_chooser =
      MwDirectoryChooser(self->main_window, "Scan a Directory");
  MwAddUserHandler(self->directory_chooser, MwNdirectoryChosenHandler,
                   file_callback, self);
};
