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

void addBug() {
    string title, description, priority;
    cout << "Title: "; getline(cin, title);
    cout << "Description: "; getline(cin, description);
    cout << "Priority (Low, Medium, High): "; getline(cin, priority);

    string sql = "INSERT INTO bugs (Title, Description, Priority) VALUES ('" +
        title + "', '" + description + "', '" + priority + "');";
    executeSQL(sql);
    cout << "Bug added.\n";
}

int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++)
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    cout << "------------------------\n";
    return 0;
}

void listBugs() {
    string sql = "SELECT * FROM bugs;";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, nullptr);
}

void updateBug() {
    string id, newStatus;
    cout << "Bug ID to update: "; getline(cin, id);
    cout << "New Status (Open/In Progress/Resolved): "; getline(cin, newStatus);

    string sql = "UPDATE bugs SET status = '" + newStatus + "' WHERE ID = " + id + ";";
    executeSQL(sql);
    cout << "Bug updated.\n";
}

void deleteBug() {
    string id;
    cout << "Bug ID to delete: "; getline(cin, id);

    string sql = "DELETE FROM bugs WHERE ID = " + id + ";";
    executeSQL(sql);
    cout << "Bug deleted.\n";
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
        getline(cin, choice);
        if (choice == "1") addBug();
        else if (choice == "2") listBugs();
        else if (choice == "3") updateBug();
        else if (choice == "4") deleteBug();
        else if (choice == "5") break;
        else cout << "Invalid option.\n";
    }

    sqlite3_close(db);
    return 0;
}

