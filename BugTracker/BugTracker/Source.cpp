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

void menu() {
    cout << "\n1. Add Bug\n2. List Bugs\n3. Update Bug\n4. Delete Bug\n5. Exit\nChoice: ";
}

int main() {
    int rc = sqlite3_open("bugs.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    createTable();
    string choice;

    while (true) {
        menu();
    }

    sqlite3_close(db);
    return 0;
}
