string sql = "INSERT INTO bugs (title, description, priority) VALUES ('" +
        title + "', '" + description + "', '" + priority + "');";
    executeSQL(sql);
    cout << "Bug added.\n";

Was the original code but was edited due to concerns of SQL injection. Additionally, the title and description did not accept whitespaces so changes were made.

*Never concatenate user input directly into SQL strings
Instead: sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT); (prepared statement with parameter bindings)
