#pragma once
#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>
#include <filesystem>
#include <string>
#include "sqlite3.h"
#include <iostream>
#include <queue>

#define BUFSIZE 1024
#define MD5LEN  16
const std::string  DB_FILENAME = "HashDB.db";

class calculatMD5 {
public:
	calculatMD5();
	~calculatMD5() {};
	//void checkHashAndAddPath(std::string hash, std::filesystem::path path);
	void checkHashAndAddPath(const std::string& hash, const std::filesystem::path& receivedPath);
	std::string createMd5(const std::filesystem::path& entry);
	std::string getListFile();

private:
	sqlite3* db;
	
	std::string path = "";
	//static int callback2(void* data, int argc, char** argv, char** colName);
};