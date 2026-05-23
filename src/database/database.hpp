#pragma once

#include <cstdint>
#include <sqlite3.h>
#include <string>

class Database {
  sqlite3 *mDB;
  sqlite3_stmt *mStatement;
  const char *mTail;

  bool statement(const char *path,
                 void (*onError)(std::string err, void *ud) = NULL,
                 void *ud = NULL);

public:
  Database();
  ~Database();

  bool create_table(std::string name,
                    void (*onError)(std::string err, void *ud), void *ud);
  void new_entry(std::string tbl_name, std::string filename,
                 std::string keywords, uint8_t *data, size_t data_len,
                 void (*onError)(std::string err, void *ud), void *ud);
};
