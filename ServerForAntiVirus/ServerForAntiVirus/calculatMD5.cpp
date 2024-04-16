
#include "calculatMD5.h"

calculatMD5::calculatMD5() {
    int rc = sqlite3_open(DB_FILENAME.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
    }
}
void calculatMD5::checkHashAndAddPath(const std::string& hash, const std::filesystem::path& receivedPath) {
    int result = 0;
    char* errMsg = nullptr;

    // Construct the SQL query with a placeholder for hash
    std::string query = "SELECT COUNT(*) FROM HashDB WHERE hash = ?";

    // Prepare the statement
    sqlite3_stmt* stmt;
    int res = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << std::endl;
        this->path = "its fine";
        return;
    }

    // Bind the hash parameter
    res = sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);
    if (res != SQLITE_OK) {
        std::cerr << "Error binding parameter: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        this->path = "its fine";
        return;
    }

    // Execute the query
    res = sqlite3_step(stmt);
    if (res == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }
    else if (res != SQLITE_DONE) {
        std::cerr << "Error executing SQL statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        this->path = "its fine";
        return;
    }

    // Finalize the statement
    sqlite3_finalize(stmt);

    if (result > 0) {
        this->path = receivedPath.string();
    }
    else {
        this->path = "its fine";
    }
}

/*
void calculatMD5::checkHashAndAddPath(const std::string& hash, const std::filesystem::path& receivedPath) {

    int result;
    char* errMsg = nullptr;
    // Construct the SQL query with a placeholder for hash
    std::string query = "SELECT COUNT(*) FROM HashDB WHERE hash ='" + hash+"'";

    // Execute the query
    int res = sqlite3_exec(db, query.c_str(), callback2, &result, &errMsg);
    std::cout << "resultM: " << result << std::endl;
    if (res != SQLITE_OK) {
        std::cerr << "Error executing SQL statement: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        this->path = "its fine";
    }
    if (result > 0) {
        this->path = receivedPath.string();
    }
    else {
        this->path = "its fine";
    }
}

int calculatMD5::callback2(void* data, int argc, char** argv, char** colName) {
    int* result = static_cast<int*>(data);
    if (argc > 0) {
        *result = std::stoi(argv[0]);
        std::cout << "resultM2: " << *result << std::endl;

    }
    return 0;
}
*/




std::string calculatMD5::createMd5(const std::filesystem::path& entry)
{
    std::string n = "";
    std::wstring wideFilePath = entry.native();
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";
    LPCWSTR filename = wideFilePath.c_str();
    // Logic to check usage goes here.

    hFile = CreateFile(filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        printf("Error opening file %s\nError: %d\n", filename,
            dwStatus);
        std::cout << dwStatus << std::endl;
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus);
        CloseHandle(hFile);
        std::cout << dwStatus << std::endl;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus);
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        std::cout << dwStatus << std::endl;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUFSIZE,
        &cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\n", dwStatus);
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            std::cout << dwStatus << std::endl;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\n", dwStatus);
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        std::cout << dwStatus << std::endl;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        printf("MD5 hash of file %s is: ", filename);
        for (DWORD i = 0; i < cbHash; i++)
        {
            n += rgbDigits[rgbHash[i] >> 4];
            n += rgbDigits[rgbHash[i] & 0xf];
            printf("%c%c", rgbDigits[rgbHash[i] >> 4],
                rgbDigits[rgbHash[i] & 0xf]);
        }
        printf("\n");
    }
    else
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\n", dwStatus);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    std::cout << "maor:" << n << std::endl;
    return n;
}

std::string calculatMD5::getListFile()
{
    return this->path;
}
