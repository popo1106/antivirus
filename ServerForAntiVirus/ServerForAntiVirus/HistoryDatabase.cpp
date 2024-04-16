
#include "HistoryDatabase.h"
#include <iostream>

HistoryDatabase::HistoryDatabase() {
    int rc = sqlite3_open("history.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
    }
    else {
        std::cout << "Opened database successfully" << std::endl;

        const char* createTableSQL = "CREATE TABLE IF NOT EXISTS History (date TEXT, time TEXT, path TEXT, result TEXT);";
        char* errMsg;
        rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
}

HistoryDatabase::~HistoryDatabase() {
    sqlite3_close(db);
}

void HistoryDatabase::insertData(const std::string& date, const std::string& time, const std::string& path, const std::string& result) {
    std::string insertSQL = "INSERT INTO History (date, time, path, result) VALUES ('" + date + "', '" + time + "', '" + path + "', '" + result + "');";
    char* errMsg;
    int rc = sqlite3_exec(db, insertSQL.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        std::cout << "Data inserted successfully" << std::endl;
    }
}


std::string HistoryDatabase::getData() {
    std::string selectSQL = "SELECT * FROM History;";
    char* errMsg;
    std::string data = "";
    int rc = sqlite3_exec(db, selectSQL.c_str(), callback, (void*)&data, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    else {
        if (data.empty()) {
            data = "The history database is empty.";
        }
    }
    return data;
}


int HistoryDatabase::callback(void* data, int argc, char** argv, char** azColName) {
    std::string* jsonData = reinterpret_cast<std::string*>(data);
    std::string row = "{";
    for (int i = 0; i < argc; i++) {
        if (i > 0) {
            row += ",";
        }
        row += azColName[i];
        row += ":";
        row += argv[i];
    }
    row += "}";
    if (!jsonData->empty()) {
        *jsonData += ",";
    }
    *jsonData += row;
    return 0;
}

