#include "gui.hpp"
#include <dbus/dbus.h>

static const char *dbgstr_dbus_error(const DBusError *error) {
  char *err = new char[255];
  snprintf(err, 255, "{%s: %s}", (error->name), (error->message));
  return err;
}

static const char *dbgstr_dbus_message(DBusMessage *message) {
  const char *interface;
  const char *member;
  const char *path;
  const char *sender;
  const char *signature;
  int type;
  char *err = new char[255];

  interface = dbus_message_get_interface(message);
  member = dbus_message_get_member(message);
  path = dbus_message_get_path(message);
  sender = dbus_message_get_sender(message);
  type = dbus_message_get_type(message);
  signature = dbus_message_get_signature(message);

  switch (type) {
  case DBUS_MESSAGE_TYPE_METHOD_CALL:
    snprintf(
        err, 255,
        "{method_call sender=%s interface=%s member=%s path=%s signature=%s}",
        sender, interface, member, path, signature);
    return err;
  case DBUS_MESSAGE_TYPE_SIGNAL:
    snprintf(err, 255,
             "{signal sender=%s interface=%s member=%s path=%s signature=%s}",
             sender, interface, member, path, signature);
    return err;
  default:
    snprintf(err, 255, "%p", message);
    return err;
  }
}

GUI::DBusContext::DBusContext() {
  DBusError error;

  if (mConn != NULL)
    return;

  dbus_threads_init_default();
  dbus_error_init(&error);

  mConn = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
  if (!mConn) {
    printf("Failed to get system dbus connection: %s\n",
           dbgstr_dbus_error(&error));
    dbus_error_free(&error);
    mValid = false;
    return;
  }

  mValid = true;
}

static DBusHandlerResult
file_chooser_response_handler(DBusConnection *connection, DBusMessage *message,
                              void *data) {
  struct file_chooser_handler_ctx *ctx =
      (struct file_chooser_handler_ctx *)data;
  DBusMessageIter margs, results, entry, variant, uris;
  const char *key, *uri;

  dbus_message_iter_init(message, &margs);
  dbus_message_iter_get_basic(&margs, &ctx->status);

  // Recurse into results
  dbus_message_iter_next(&margs);
  dbus_message_iter_recurse(&margs, &results);
  for (;;) {
    dbus_message_iter_recurse(&results, &entry);

    dbus_message_iter_get_basic(&entry, &key);
    dbus_message_iter_next(&entry);
    dbus_message_iter_recurse(&entry, &variant);

    if (strcmp(key, "uris") == 0) {
      dbus_message_iter_recurse(&variant, &uris);
      dbus_message_iter_get_basic(&uris, &uri);

      printf("%p\n", ctx->callback);
      ctx->callback(NULL, ctx->gui, (void *)uri);
      break;
    }

    if (!dbus_message_iter_has_next(&results))
      break;
    dbus_message_iter_next(&results);
  }

  return DBUS_HANDLER_RESULT_HANDLED;
}

static const DBusObjectPathVTable file_chooser_response_vtable = {
    .message_function = file_chooser_response_handler,
};

bool GUI::DBusContext::open_file(GUI *gui, MwUserHandler handler) {
  DBusMessage *msg;
  DBusMessageIter margs;
  DBusMessageIter options;
  DBusPendingCall *pending;
  DBusError error;
  const char *handle;

  dbus_error_init(&error);

  msg = dbus_message_new_method_call(
      "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.FileChooser", "OpenFile");

  if (msg == NULL) {
    printf("no memory\n");
    return false;
  }
  const char *parent_window = "";
  const char *title = "Choose a directory";

  // msg != NULL
  dbus_message_iter_init_append(msg, &margs);
  dbus_message_iter_append_basic(&margs, DBUS_TYPE_STRING,
                                 &parent_window); // parent_window
  dbus_message_iter_append_basic(&margs, DBUS_TYPE_STRING,
                                 &title); // title
  dbus_message_iter_open_container(&margs, 'a', "{sv}", &options);
  {
    DBusMessageIter entry, value;
    const char *key = "directory";
    dbus_bool_t dir = TRUE;

    dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, NULL,
                                     &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &value);
    dbus_message_iter_append_basic(&value, DBUS_TYPE_BOOLEAN, &dir);
    dbus_message_iter_close_container(&entry, &value);
    dbus_message_iter_close_container(&options, &entry);
  }

  dbus_message_iter_close_container(&margs, &options);

  if (!dbus_connection_send_with_reply(mConn, msg, &pending, -1)) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&error));
    dbus_error_free(&error);
    dbus_message_unref(msg);
    return false;
  }

  if (pending == NULL) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&error));
    dbus_error_free(&error);
    dbus_message_unref(msg);
    return false;
  }

  dbus_connection_flush(mConn);
  dbus_message_unref(msg);

  // Wait for response
  dbus_pending_call_block(pending);
  msg = dbus_pending_call_steal_reply(pending);
  if (msg == NULL) {
    printf("Did not get pending call response for file dialog request: %s\n",
           dbgstr_dbus_error(&error));
    dbus_error_free(&error);
    dbus_pending_call_unref(pending);
    return false;
  }
  dbus_pending_call_unref(pending);

  // Read handle
  dbus_message_iter_init(msg, &margs);
  dbus_message_iter_get_basic(&margs, &handle);

  dbus_message_unref(msg);

  mHandlerContext.status = -1;
  mHandlerContext.callback = handler;
  mHandlerContext.gui = gui;
  printf("%p\n", mHandlerContext.callback);

  dbus_connection_try_register_object_path(
      mConn, handle, &file_chooser_response_vtable, &mHandlerContext, &error);

  return true;
}
