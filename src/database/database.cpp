#include "database.hpp"
#include <algorithm>
#include <csignal>
#include <cstring>
#include <filesystem>

Database::Database() {
  char dir[2048];
#ifdef __linux__
  snprintf(dir, sizeof(dir), "%s/.sortmancer/", getenv("HOME"));
#elif _WIN32
  /* TODO: I'm pretty sure this works? but it's untested.  */
  snprintf(dir, sizeof(dir), "%s\\.sortmancer\\", getenv("HOMEPATH"));
#else
#error Unknown platform
#endif
  std::filesystem::create_directories(dir);

  std::filesystem::path path = std::filesystem::path(dir) / "sortmancer.db";
  if (sqlite3_open(path.c_str(), &mDB) != SQLITE_OK) {
    char error[2048];
    memset(error, 0, sizeof(error));
    snprintf(error, sizeof(error) - 1, "Error opening sortmancer.db: %s",
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
           "CREATE VIRTUAL TABLE IF NOT EXISTS \"%s\" USING fts5(filename, "
           "keywords);",
           table_name.c_str());

  return this->statement(path, onError, ud);
}
void Database::new_entry(std::string table_name, std::string filename,
                         std::string keywords,
                         void (*onError)(std::string err, void *ud), void *ud) {
  char sql[2048];
  memset(sql, 0, sizeof(sql));
  snprintf(sql, sizeof(sql) - 1,
           "INSERT INTO \"%s\" (filename, keywords) VALUES ("
           "\"%s\", \"%s\""
           ");",
           table_name.c_str(), filename.c_str(), keywords.c_str());

  this->statement(sql);
  return;
};

std::vector<Database::DatabaseEntry>
Database::search(std::string search_str,
                 void (*onError)(std::string err, void *ud), void *ud) {
  int err;
  std::string tbl_query =
      "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
  std::vector<std::string> tables;
  std::vector<Database::DatabaseEntry> entries;

  /* list all tables */
  if ((err = sqlite3_prepare_v2(mDB, tbl_query.c_str(), tbl_query.length(),
                                &mStatement, &mTail)) != SQLITE_OK) {
    goto err;
  };
  err = sqlite3_step(mStatement);

  while (err == SQLITE_OK || err == SQLITE_ROW) {
    const char *result = (const char *)sqlite3_column_text(mStatement, 0);
    if (result) {
      std::string tbl_name = result;
      if (tbl_name.find("_config") == std::string::npos &&
          tbl_name.find("_content") == std::string::npos &&
          tbl_name.find("_data") == std::string::npos &&
          tbl_name.find("_docsize") == std::string::npos &&
          tbl_name.find("_idx") == std::string::npos) {
        tables.push_back(tbl_name);
      }
    }

    err = sqlite3_step(mStatement);
  };

  for (std::string col_name : {"keywords", "filename"}) {
    /* keyword filtering */
    for (auto table : tables) {
      std::string row_query = "SELECT filename, keywords FROM \"" + table +
                              "\" WHERE " + col_name + " MATCH \'" +
                              search_str + "\';";
      if ((err = sqlite3_prepare_v2(mDB, row_query.c_str(), row_query.length(),
                                    &mStatement, &mTail)) != SQLITE_OK) {
        printf("Error on %s: %s", table.c_str(), sqlite3_errmsg(mDB));
        continue;
      };
      err = sqlite3_step(mStatement);

      while (err == SQLITE_OK || err == SQLITE_ROW) {
        const char *filename = (const char *)sqlite3_column_text(mStatement, 0);
        const char *keywords = (const char *)sqlite3_column_text(mStatement, 1);

        entries.push_back(Database::DatabaseEntry{
            .table_name = table.c_str(),
            .filename = filename,
            .keywords = keywords,
        });

        err = sqlite3_step(mStatement);
      };
    }
  }

  std::sort(
      entries.begin(), entries.end(),
      [&](Database::DatabaseEntry const &a, Database::DatabaseEntry const &b) {
        return a.keywords.find(search_str) < b.keywords.find(search_str);
      });

  return entries;

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
  return entries;
};

Database::~Database() { sqlite3_close(mDB); }
