#pragma once
#include "../database/database.hpp"
#include "../devices/devices.hpp"
#include "../model/model.hpp"
#include "../thread/thread.hpp"
#include <Mw/Milsko.h>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>

#ifdef __linux__
#include <dbus/dbus.h>
struct file_chooser_handler_ctx {
  int status;
  MwUserHandler callback;
  class GUI *gui;
  void (*onError)(std::string err, void *ud);
  void *onError_ud;
};
#endif

class GUI {
  /* Thread pool with <8 threads */
  BS::thread_pool<> mPool =
      BS::thread_pool<>((std::thread::hardware_concurrency() < 8)
                            ? std::thread::hardware_concurrency()
                            : 8);

public:
#ifdef __linux__
  class DBusContext {
    bool mValid = false;
    DBusError mErr;
    DBusConnection *mConn = NULL;
    DBusMessage *mMessage = NULL;
    DBusMessage *mReply = NULL;

    struct file_chooser_handler_ctx mHandlerContext;

  public:
    char handle_path[256];

    typedef void (*DBusPortalPollListener)(void *handle, MwU32 new_value);

    DBusContext();
    // ~DBusContext();

    bool valid() { return mValid; };

    int status() { return mHandlerContext.status; };

    bool open_file(GUI *gui, MwUserHandler handler,
                   void (*onError)(std::string err, void *ud), void *ud);
    DBusConnection *conn() { return mConn; }
  };
#endif

  Database db;
  DBusContext dbus;

  class MainWindow {
  public:
    GUI *gui;
    MainWindow(GUI *gui);
    ~MainWindow();
    MwWidget main_window = NULL;
    MwWidget main_box = NULL;
    MwWidget tab_view = NULL;
    MwWidget search_results_box = NULL;
    MwWidget search_box_holder = NULL;
    MwWidget search_box_text = NULL;
    MwWidget search_box = NULL;
    MwWidget search_box_button = NULL;
    MwWidget search_results_listbox = NULL;

    MwWidget device_scan_button_holder = NULL;
    MwWidget device_scan_button = NULL;
    MwWidget directory_chooser = NULL;
    static void resize(MwWidget handle, void *user_data, void *call_data);
    static void window_tick(MwWidget widget, void *user, void *client);
    static void search_btn(MwWidget handle, void *user_data, void *call_data);
  };
  MainWindow *main_window;

  MwWidget device_window = NULL;
  MwWidget device_list = NULL;
  MwWidget device_warning = NULL;
  MwWidget device_listbox = NULL;
  MwWidget device_window_browse = NULL;

  ModelContext *modelContext = NULL;

#ifdef __linux__
  void start_dbus_filechooser(MwUserHandler handler);
#endif

  std::mutex stdoutMutex;

  GUI();

  std::mutex errMutex;

  std::mutex tickMutex;
  std::unordered_map<std::filesystem::path, char> scannedFiles;
  struct ScanCreationEntry {
    int idx;
    char dir[255];
    std::string labelName;
  };
  struct ScanLine {
    MwWidget box = NULL;
    MwWidget tab = NULL;
    int count = 0;
  };
  struct ScanEntry {
    int idx = 0;
    char line1[255];
    char line2[255];
  };
  std::vector<ScanCreationEntry> scanBoxCreationQueue;
  std::vector<ScanEntry> scanBoxEntryQueue;
  std::vector<ScanLine> scanLines;

  std::vector<std::thread *> scanThreads;
  std::vector<std::thread *> scanStarterThreads;
  std::vector<Device> scannedDevices;
  std::vector<std::string> errorCreationQueue;
  std::vector<ScanCreationEntry> scanStartQueue;
  std::string scanDeviceName;

  void start_scan(std::string dir, std::string tbl);

  static void file_choose_button_handler(MwWidget widget, void *user,
                                         void *client);
  static void device_choose_button_handler(MwWidget widget, void *user,
                                           void *client);
};

void dir_recurse(const std::string &path,
                 std::function<void(const std::filesystem::path &)> cb);

static const std::string avail_file_exts[] = {
    ".apng", ".png",   ".avif", ".gif", ".jpg", ".jpeg",
    ".jfif", ".pjpeg", ".pjp",  ".png", ".svg", ".webp"};
