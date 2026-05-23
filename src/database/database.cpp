#include "database.hpp"
#include <csignal>
#include <cstring>
#include <filesystem>

Database::Database() {
  char dir[2048];
#ifdef __linux__
  snprintf(dir, sizeof(dir), "%s/.spcf/", getenv("HOME"));
#elif _WIN32
  /* TODO: I'm pretty sure this works? but it's untested.  */
  snprintf(dir, sizeof(dir), "%s\\.spcf\\", getenv("HOMEPATH"));
#else
#error Unknown platform
#endif
  std::filesystem::create_directories(dir);

  std::filesystem::path path = std::filesystem::path(dir) / "spcf.db";
  if (sqlite3_open(path.c_str(), &mDB) != SQLITE_OK) {
    char error[2048];
    memset(error, 0, sizeof(error));
    snprintf(error, sizeof(error) - 1, "Error opening spcf.db: %s",
             sqlite3_errmsg(mDB));
    printf("%s\n", error);
#ifdef __linux__
    raise(SIGTRAP);
#else
    printf("%s\n", error);
    exit(1);
#endif
  };
}

bool Database::statement(const char *sql,
                         void (*onError)(std::string err, void *ud), void *ud) {
  int err;
  if ((err = sqlite3_prepare_v2(mDB, sql, strlen(sql), &mStatement, &mTail)) !=
      SQLITE_OK) {
    goto err;
  };
  err = sqlite3_step(mStatement);
  if (err != SQLITE_OK && err != SQLITE_DONE) {
    goto err;
  };

  return true;

err:
  char error[2048];
  memset(error, 0, sizeof(error));
  snprintf(error, sizeof(error) - 1, "Database error: %s", sqlite3_errmsg(mDB));
  printf("%s\n", error);
  if (onError) {
    onError(error, ud);
  } else {
#ifdef __linux__
    raise(SIGTRAP);
#else
    exit(1);
#endif
  }
  return false;
};

bool Database::create_table(std::string table_name,
                            void (*onError)(std::string err, void *ud),
                            void *ud) {
  char path[2048];
  memset(path, 0, sizeof(path));
  snprintf(path, sizeof(path) - 1,
           "CREATE TABLE IF NOT EXISTS \"%s\" ("
           "id INTEGER PRIMARY KEY,"
           "filename TEXT NOT NULL,"
           "keywords TEXT NOT NULL,"
           "data BLOB"
           ");",
           table_name.c_str());

  return this->statement(path, onError, ud);
}
void Database::new_entry(std::string table_name, std::string filename,
                         std::string keywords, uint8_t *data, size_t data_len,
                         void (*onError)(std::string err, void *ud), void *ud) {
  char sql[2048];
  memset(sql, 0, sizeof(sql));
  snprintf(sql, sizeof(sql) - 1,
           "INSERT INTO \"%s\" (filename, keywords, data) VALUES ("
           "\"%s\", \"%s\", ?"
           ");",
           table_name.c_str(), filename.c_str(), keywords.c_str());

  int err;
  if ((err = sqlite3_prepare_v2(mDB, sql, strlen(sql), &mStatement, &mTail)) !=
      SQLITE_OK) {
    goto err;
  };
  err = sqlite3_bind_blob(mStatement, 1, data, data_len, SQLITE_TRANSIENT);
  if (err != SQLITE_OK && err != SQLITE_DONE) {
    goto err;
  };

  err = sqlite3_step(mStatement);
  if (err != SQLITE_OK && err != SQLITE_DONE) {
    goto err;
  };

  return;

err:
  char error[2048];
  memset(error, 0, sizeof(error));
  snprintf(error, sizeof(error) - 1, "Database error: %s", sqlite3_errmsg(mDB));
  printf("%s\n", error);
  if (onError) {
    onError(error, ud);
  } else {
#ifdef __linux__
    raise(SIGTRAP);
#else
    exit(1);
#endif
  }
  return;
};
Database::~Database() { sqlite3_close(mDB); }
