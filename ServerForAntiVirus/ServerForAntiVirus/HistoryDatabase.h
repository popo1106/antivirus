#pragma once
#ifndef HISTORYDATABASE_H
#define HISTORYDATABASE_H

#include "sqlite3.h"
#include <string>

class HistoryDatabase {
private:
    sqlite3* db;
    static std::wstring data;
public:
    HistoryDatabase();
    ~HistoryDatabase();
    void insertData(const std::string& date, const std::string& time, const std::string& path, const std::string& result);
    std::string getData();

private:
    static int callback(void* data, int argc, char** argv, char** azColName);

};

#endif // HISTORYDATABASE_H
