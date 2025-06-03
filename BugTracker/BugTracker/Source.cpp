extern "C" {
#include "sqlite3.h"
}
#include <iostream>
#include <string>
#include <algorithm>
#include <regex>

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

// Validation functions
bool isValidTitle(const string& title) {
    // Title should not be empty and should be less than 100 characters
    return !title.empty() && title.length() <= 100;
}

bool isValidDescription(const string& description) {
    // Description should not be empty and should be less than 1000 characters
    return !description.empty() && description.length() <= 1000;
}

bool isValidPriority(const string& priority) {
    // Convert to lowercase for case-insensitive comparison
    string lowerPriority = priority;
    transform(lowerPriority.begin(), lowerPriority.end(), lowerPriority.begin(), ::tolower);
    return lowerPriority == "low" || lowerPriority == "medium" || lowerPriority == "high";
}

bool isValidStatus(const string& status) {
    // Convert to lowercase for case-insensitive comparison
    string lowerStatus = status;
    transform(lowerStatus.begin(), lowerStatus.end(), lowerStatus.begin(), ::tolower);
    return lowerStatus == "open" || lowerStatus == "in progress" || lowerStatus == "resolved";
}

bool isValidBugId(const string& id) {
    // Check if the ID is a positive integer
    regex idPattern("^[1-9][0-9]*$");
    return regex_match(id, idPattern);
}

string getValidInput(const string& prompt, const string& errorMsg, bool (*validator)(const string&)) {
    string input;
    while (true) {
        cout << prompt;
        getline(cin, input);
        if (validator(input)) {
            return input;
        }
        cout << errorMsg << endl;
    }
}

void addBug() {
    string title = getValidInput("Title: ", 
        "Invalid title. Title must not be empty and must be less than 100 characters.", 
        isValidTitle);
    
    string description = getValidInput("Description: ", 
        "Invalid description. Description must not be empty and must be less than 1000 characters.", 
        isValidDescription);
    
    string priority = getValidInput("Priority (Low, Medium, High): ", 
        "Invalid priority. Please enter Low, Medium, or High.", 
        isValidPriority);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO bugs (Title, Description, Priority) VALUES (?, ?, ?);";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, priority.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to insert bug: " << sqlite3_errmsg(db) << endl;
    } else {
        cout << "Bug added.\n";
    }

    sqlite3_finalize(stmt);
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

bool bugExists(const string& id) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM bugs WHERE ID = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    
    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    return exists;
}

void updateBug() {
    string id = getValidInput("Bug ID to update: ", 
        "Invalid ID. Please enter a positive number.", 
        isValidBugId);
    
    if (!bugExists(id)) {
        cout << "Error: Bug with ID " << id << " does not exist.\n";
        return;
    }
    
    string newStatus = getValidInput("New Status (Open/In Progress/Resolved): ", 
        "Invalid status. Please enter Open, In Progress, or Resolved.", 
        isValidStatus);

    sqlite3_stmt* stmt;
    const char* sql = "UPDATE bugs SET status = ? WHERE ID = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to update bug: " << sqlite3_errmsg(db) << endl;
    } else {
        cout << "Bug updated.\n";
    }

    sqlite3_finalize(stmt);
}

void deleteBug() {
    string id = getValidInput("Bug ID to delete: ", 
        "Invalid ID. Please enter a positive number.", 
        isValidBugId);

    if (!bugExists(id)) {
        cout << "Error: Bug with ID " << id << " does not exist.\n";
        return;
    }

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM bugs WHERE ID = ?;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        cerr << "Failed to delete bug: " << sqlite3_errmsg(db) << endl;
    } else {
        cout << "Bug deleted.\n";
    }

    sqlite3_finalize(stmt);
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

