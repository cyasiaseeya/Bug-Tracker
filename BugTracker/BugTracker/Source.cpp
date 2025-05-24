extern "C" {
#include "sqlite3.h"
}
#include <iostream>
#include <string>

using namespace std;

sqlite3* db;

void executeSQL(const string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL Error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

void createTable() {
    string sql = R"(CREATE TABLE IF NOT EXISTS bugs (
        ID INTEGER PRIMARY KEY AUTOINCREMENT,
        Title TEXT NOT NULL,
        Description TEXT,
        Status TEXT DEFAULT 'Open',
        Priority TEXT,
        Date TEXT DEFAULT CURRENT_DATE
    );)";
    executeSQL(sql);
}