// Include SQLite3 C API
extern "C" {
#include "sqlite3.h"
}
#include <iostream>
#include <string>
#include <algorithm>
#include <regex>

using namespace std;

// Global database connection pointer
sqlite3* db;

/**
 * Executes a SQL query and handles any errors that occur
 * @param sql The SQL query string to execute
 */
void executeSQL(const string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL Error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

/**
 * Creates the bugs table if it doesn't exist
 * Defines the schema with columns for ID, Title, Description, Status, Priority, and Date
 */
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

/**
 * Validates if a title meets the requirements
 * @param title The title to validate
 * @return true if title is valid (not empty and <= 100 chars), false otherwise
 */
bool isValidTitle(const string& title) {
    return !title.empty() && title.length() <= 100;
}

/**
 * Validates if a description meets the requirements
 * @param description The description to validate
 * @return true if description is valid (not empty and <= 1000 chars), false otherwise
 */
bool isValidDescription(const string& description) {
    return !description.empty() && description.length() <= 1000;
}

/**
 * Validates if a priority value is valid
 * @param priority The priority to validate
 * @return true if priority is "low", "medium", or "high" (case-insensitive), false otherwise
 */
bool isValidPriority(const string& priority) {
    string lowerPriority = priority;
    transform(lowerPriority.begin(), lowerPriority.end(), lowerPriority.begin(), ::tolower);
    return lowerPriority == "low" || lowerPriority == "medium" || lowerPriority == "high";
}

/**
 * Validates if a status value is valid
 * @param status The status to validate
 * @return true if status is "open", "in progress", or "resolved" (case-insensitive), false otherwise
 */
bool isValidStatus(const string& status) {
    string lowerStatus = status;
    transform(lowerStatus.begin(), lowerStatus.end(), lowerStatus.begin(), ::tolower);
    return lowerStatus == "open" || lowerStatus == "in progress" || lowerStatus == "resolved";
}

/**
 * Validates if a bug ID is a valid positive integer
 * @param id The ID to validate
 * @return true if ID is a positive integer, false otherwise
 */
bool isValidBugId(const string& id) {
    regex idPattern("^[1-9][0-9]*$");
    return regex_match(id, idPattern);
}

/**
 * Generic input validation function that keeps prompting until valid input is received
 * @param prompt The prompt to display to the user
 * @param errorMsg The error message to display for invalid input
 * @param validator Function pointer to the validation function to use
 * @return The validated input string
 */
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

/**
 * Checks if a bug with the given ID exists in the database
 * @param id The bug ID to check
 * @return true if the bug exists, false otherwise
 */
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

/**
 * Adds a new bug to the database
 * Prompts for and validates title, description, and priority
 * Uses parameterized queries to prevent SQL injection
 */
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

/**
 * Callback function for SQLite queries that prints each row of results
 * @param NotUsed Unused parameter required by SQLite
 * @param argc Number of columns in the result
 * @param argv Array of column values
 * @param azColName Array of column names
 * @return 0 to continue query execution
 */
int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++)
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    cout << "------------------------\n";
    return 0;
}

/**
 * Lists all bugs in the database
 * Uses the callback function to display each bug's details
 */
void listBugs() {
    string sql = "SELECT * FROM bugs;";
    sqlite3_exec(db, sql.c_str(), callback, nullptr, nullptr);
}

/**
 * Updates the status of an existing bug
 * Validates the bug ID and new status
 * Uses parameterized queries to prevent SQL injection
 */
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

/**
 * Deletes a bug from the database
 * Validates the bug ID and checks if the bug exists
 * Uses parameterized queries to prevent SQL injection
 */
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

/**
 * Displays the main menu options
 */
void menu() {
    cout << "\n1. Add Bug\n2. List Bugs\n3. Update Bug\n4. Delete Bug\n5. Exit\nChoice: ";
}

/**
 * Main function that initializes the database and runs the main program loop
 * @return 0 on successful execution, 1 on database initialization failure
 */
int main() {
    // Open database connection
    int rc = sqlite3_open("bugs.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    }

    // Create the bugs table if it doesn't exist
    createTable();
    string choice;

    // Main program loop
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

    // Clean up database connection
    sqlite3_close(db);
    return 0;
}

