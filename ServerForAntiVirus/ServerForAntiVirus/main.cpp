#include "Server.h"
#include "getFile.h"
#include "calculatMD5.h"
#include "HistoryDatabase.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>
#include <ctime>
#include <io.h>
#include <fcntl.h>
#include <Dbt.h>
#include <filesystem>
#include <set>
#ifndef FILE_REMOVABLE_MEDIA
#define FILE_REMOVABLE_MEDIA 0x00000004
#endif
#include <map>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

void scheduleFunction(std::atomic<bool>& flag, Server& server);
std::string queueToString(std::queue<std::string>& q);
std::queue<std::string> printQueue(std::queue<std::filesystem::path>& q);
void management(std::pair<int, std::string> result, Server& server);
std::pair<int, std::string> extractRequestAndInfo(const std::string& input);
std::string scanFile(std::string path);
void extractHourMinutePath(const std::string& str);
std::string getCurrentDate();
std::string getCurrentTime();
void checkUSB(Server& server);
std::vector<fs::path> listFilesOnDrive(const std::string& drive);
bool isRemovableDriveConnectedViaUSB(const std::string& drivePath);

int scheduledMinute = 0;
int scheduledHour = 0;
std::string pathAuto = "";
std::atomic<bool> flag{ false };

int main() {
    try {
        Server server;
        std::atomic<bool> serverRunning(true);
        std::mutex serverRunningMutex;
        std::unique_lock<std::mutex> serverRunningLock(serverRunningMutex);
        std::thread scheduler(scheduleFunction, std::ref(flag), std::ref(server));
        std::thread usbChecker(checkUSB, std::ref(server)); // Start USB checker thread

        while (serverRunning) {
            try {
                server.startHandleRequests();
                while (true) {
                    std::string clientRequest = server.receiveInformation();
                    std::cout << "Received request from client: " << clientRequest << std::endl;
                    std::pair<int, std::string>  result = extractRequestAndInfo(clientRequest);
                    std::cout << "Request Number: " << result.first << std::endl;
                    std::cout << "Information: " << result.second << std::endl;
                    management(result, server);
                    if (clientRequest.empty()) {
                        break;
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception occurred: " << e.what() << std::endl;
                if (e.what() == std::string("Connection closed by client.")) {
                    std::cout << "Client disconnected. Waiting for client to reconnect..." << std::endl;
                    server.CloseClientSocket();
                }
            }
        }

        server.Close();
        WSACleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    return 0;
}

void management(std::pair<int, std::string> result, Server& server) {
    try {
        HistoryDatabase history;
        if (result.first == SCAN) {
            std::string msg = scanFile(result.second);
            history.insertData(getCurrentDate(), getCurrentTime(), result.second, msg);
            std::cout << "me and you2:" << msg << std::endl;
            server.SendMessageW(msg);
        }
        else if (result.first == AUTO) {
            extractHourMinutePath(result.second);
            getFile getfile;
            if (!getfile.folderIsExists(pathAuto)) {
                server.SendMessageW("No such folder exists!!!!");
            }
            else {

                server.SendMessageW("This folder exists");
                flag.store(true);
            }
        }
        else if (result.first == HISTORY) {
            server.SendMessageW(history.getData());
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred in management function: " << e.what() << std::endl;
    }
}

void scheduleFunction(std::atomic<bool>& flag, Server& server) {
    while (true) {
        // Get the current time
        auto now = std::chrono::system_clock::now();
        time_t currentTime = std::chrono::system_clock::to_time_t(now);
        struct tm localTime;
        localtime_s(&localTime, &currentTime); // Use localtime_s instead of localtime
        // Check if the flag is true and if the current time matches the desired time
        if (flag.load() && localTime.tm_hour == scheduledHour && localTime.tm_min == scheduledMinute) {
            // Call your function
            std::string msg = scanFile(pathAuto);
            //server.SendMessageW(msg);
            std::cout << "heelo its time to go?" << std::endl;
            HistoryDatabase history;
            history.insertData(getCurrentDate(), getCurrentTime(), pathAuto, msg);
            // Reset the flag back to false until the next scheduled time
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }

        // Sleep for some time before checking again (e.g., every minute)
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void extractHourMinutePath(const std::string& str) {
    // Find the positions of ':' and ','
    size_t colonPos = str.find(':');
    size_t commaPos = str.find(',');

    // Extract hour substring
    scheduledHour = stoi(str.substr(1, colonPos - 1));

    // Extract minute substring
    scheduledMinute = stoi(str.substr(colonPos + 1, commaPos - colonPos - 1));

    // Extract path substring
    pathAuto = str.substr(commaPos + 1, str.size() - commaPos - 2);
}

void checkUSB(Server& server) {
    std::map<char, std::set<DWORD>> driveSerialNumbers;
    std::set<char> checkedDrives;
    try {
        while (true) {
            DWORD drives = GetLogicalDrives();
            char driveLetter = 'A';
            std::set<char> connectedDrives;

            while (drives) {
                if (drives & 1) {
                    std::string drivePath = std::string(1, driveLetter) + ":\\";
                    if (isRemovableDriveConnectedViaUSB(drivePath)) {
                        DWORD driveSerialNumber = 0;
                        DWORD maximumComponentLength = 0;
                        DWORD fileSystemFlags = 0;
                        char fileSystemName[MAX_PATH + 1] = { 0 };
                        if (GetVolumeInformationA(drivePath.c_str(), NULL, 0, &driveSerialNumber, &maximumComponentLength, &fileSystemFlags, fileSystemName, MAX_PATH)) {
                            connectedDrives.insert(driveLetter);
                            // If the drive has not been checked while connected before
                            if (checkedDrives.find(driveLetter) == checkedDrives.end()) {
                                driveSerialNumbers[driveLetter] = std::set<DWORD>{ driveSerialNumber };
                                std::cout << "Drive " << driveLetter << " is an external drive connected via USB." << std::endl;
                                std::vector<fs::path> filesOnDrive = listFilesOnDrive(drivePath);
                                std::cout << "Files on drive " << driveLetter << ":" << std::endl;
                                if (filesOnDrive.empty()) {
                                    std::cout << "No files found." << std::endl;
                                }
                                else {
                                    for (const auto& file : filesOnDrive) {
                                        std::string msg = scanFile(file.string());
                                        if (msg != "The files are fine" && msg != "No such folder exists!!!!")
                                        {
                                            MessageBoxA(NULL, msg.c_str(), "Error", MB_ICONERROR);
                                        }
                                        //std::cout << file.string() << std::endl;
                                    }
                                }
                                // Mark the drive as checked
                                checkedDrives.insert(driveLetter);
                            }
                        }
                    }
                    else {
                        if (driveSerialNumbers.find(driveLetter) != driveSerialNumbers.end()) {
                            // If the drive was checked before and now disconnected, remove it from checked drives
                            driveSerialNumbers.erase(driveLetter);
                        }
                    }
                }

                drives >>= 1;
                ++driveLetter;
            }

            // Check if any drives were previously checked but now disconnected and restored
            for (auto it = checkedDrives.begin(); it != checkedDrives.end();) {
                char drive = *it;
                if (connectedDrives.find(drive) == connectedDrives.end()) {
                    it = checkedDrives.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Wait for some time before checking again
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred in USB checking: " << e.what() << std::endl;
    }
}

// Function to get the current date
std::string getCurrentDate() {
    std::time_t currentTime = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &currentTime); // Use localtime_s instead of localtime

    char buffer[11]; // Buffer to hold the formatted date (e.g., "YYYY-MM-DD\0")
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);

    return std::string(buffer);
}

// Function to get the current time
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &nowTime); // Use localtime_s instead of localtime

    char buffer[9]; // Buffer to hold the formatted time (e.g., "HH:MM:SS\0")
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);

    return std::string(buffer);
}

// Function to list all files and directories on the external drive
std::vector<fs::path> listFilesOnDrive(const std::string& drive) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(drive)) {
        files.push_back(entry.path());
    }
    return files;
}

// Function to check if a drive is removable and connected via USB
bool isRemovableDriveConnectedViaUSB(const std::string& drivePath) {
    auto driveType = GetDriveTypeA(drivePath.c_str());
    if (driveType == DRIVE_REMOVABLE) {
        DWORD driveSerialNumber = 0;
        DWORD maximumComponentLength = 0;
        DWORD fileSystemFlags = 0;
        char fileSystemName[MAX_PATH + 1] = { 0 };
        if (GetVolumeInformationA(drivePath.c_str(), NULL, 0, &driveSerialNumber, &maximumComponentLength, &fileSystemFlags, fileSystemName, MAX_PATH)) {
            return (fileSystemFlags & FILE_REMOVABLE_MEDIA) != 0;
        }
    }
    return false;
}
std::pair<int, std::string> extractRequestAndInfo(const std::string& input) {
    // Extract the request number
    int requestNumber;
    if (input.size() < 2 || !isdigit(input[0]) || !isdigit(input[1])) {
        throw std::runtime_error("Invalid request format");
    }
    requestNumber = (input[0] - '0') * 10 + (input[1] - '0');

    // Extract the rest of the information
    std::string information = input.substr(2);

    return std::make_pair(requestNumber, information);
}


std::queue<std::string> printQueue(std::queue<std::filesystem::path>& q) {
    calculatMD5 getMD5;
    // Create a temporary queue to preserve the original queue
    std::queue<std::filesystem::path> temp = q;
    std::queue<std::string> listFile;


    std::cout << "Queue: ";
    // Dequeue elements and print them until the queue is empty
    while (!temp.empty()) {
        std::string md5 = getMD5.createMd5(temp.front());

        getMD5.checkHashAndAddPath(md5, temp.front());

        std::cout<<"lolq2: " << md5 << " " << std::endl;
        std::string mess = getMD5.getListFile();
        std::cout << "messss: " << mess << std::endl;
        if (mess.compare("its fine") != 0)
        {
            listFile.push(mess);
        }

        temp.pop(); // Remove the front element
    }
    std::cout << std::endl;
    return listFile;
}

std::string queueToString(std::queue<std::string>& q) {
    std::string result;

    // Iterate over the queue
    std::queue<std::string> temp = q;
    while (!temp.empty()) {
        // Append the current element to the result string
        result += temp.front();

        // Add comma if not the last element
        if (temp.size() > 1) {
            result += ",";
        }

        // Remove the front element
        temp.pop();
    }

    return result;
}
void printPathQueue(const std::queue<std::filesystem::path>& pathQueue) {
    std::cout << "Queue Contents:" << std::endl;
    std::queue<std::filesystem::path> temp = pathQueue; // Create a copy of the queue
    while (!temp.empty()) {
        std::cout << temp.front() << std::endl;
        temp.pop(); // Remove the front element
    }
}
std::string scanFile(std::string path)
{
    getFile getfile;

    std::string errMess = getfile.takeFile(path);
    if (errMess == "No such folder exists!!!!")
    {
        return errMess;
    }
    else
    {
        HistoryDatabase history;
        std::string mess = "";
        std::cout << errMess << std::endl;
        std::queue<std::filesystem::path> listFile = getfile.getQueue();
        std::cout << "scan" << std::endl;
        printPathQueue(listFile);
        std::queue<std::string> stringListFile = printQueue(listFile);
        if (stringListFile.empty())
        {
            std::cout << "its fine" << std::endl;
            mess = "The files are fine";
        }
        else
        {
            std::cout << "not good" << std::endl;
            std::string mess2 = queueToString(stringListFile);
            std::cout << "mess: " + mess << std::endl;
            mess = "The suspicious files are: " + mess2;
            
        }
        std::cout << "me and you:" << mess << std::endl;
        return mess;
    }
}
/*
#include "Server.h"
#include "getFile.h"
#include "calculatMD5.h"
#include "HistoryDatabase.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <atomic>
#include <ctime>
#include <io.h>
#include <fcntl.h>
#include <Dbt.h>
#include <filesystem>
#include <set>
#ifndef FILE_REMOVABLE_MEDIA
#define FILE_REMOVABLE_MEDIA 0x00000004
#endif
#include <map>
namespace fs = std::filesystem;

void scheduleFunction(std::atomic<bool>& flag, Server& server);
std::string queueToString(std::queue<std::string>& q);
std::queue<std::string> printQueue(std::queue<std::filesystem::path>& q);
void management(std::pair<int, std::string> result, Server& server);
std::pair<int, std::string> extractRequestAndInfo(const std::string& input);
std::string scanFile(std::string path);
//void extractTime(std::string t);
void extractHourMinutePath(const std::string& str);
std::string getCurrentDate();
std::string getCurrentTime();
void checkUSB();
std::vector<fs::path> listFilesOnDrive(const std::string& drive);
bool isRemovableDriveConnectedViaUSB(const std::string& drivePath);


int scheduledMinute = 0;
int scheduledHour = 0;
std::string pathAuto = "";
std::atomic<bool> flag{ false };

int main() {
    try {
        Server server;
        std::atomic<bool> serverRunning(true);
        std::mutex serverRunningMutex;
        std::unique_lock<std::mutex> serverRunningLock(serverRunningMutex);
        std::thread scheduler(scheduleFunction, std::ref(flag), std::ref(server));
        std::thread usbChecker(checkUSB); // Start USB checker thread

        while (serverRunning) {
            try {
                 server.startHandleRequests();
                while (true) {
                    std::string clientRequest = server.receiveInformation();
                    std::cout << "Received request from client: " << clientRequest << std::endl;
                    std::pair<int, std::string>  result = extractRequestAndInfo(clientRequest);
                    std::cout << "Request Number: " << result.first << std::endl;
                    std::cout << "Information: " << result.second << std::endl;
                    management(result, server);
                    if (clientRequest.empty()) {
                        break;
                    }
                }
            }
            catch (const std::exception& e) {
                std::wcerr << "Exception occurred: " << e.what() << std::endl;
                if (e.what() == std::string("Connection closed by client.")) {
                    std::cout << "Client disconnected. Waiting for client to reconnect..." << std::endl;
                    server.CloseClientSocket();
                }
            }
        }

        server.Close();
        WSACleanup();
    }
    catch (const std::exception& e) {
        std::wcerr << "Exception occurred: " << e.what() << std::endl;
    }

    return 0;
}


void management(std::pair<int, std::string> result, Server& server)
{
    try {
        HistoryDatabase history;
        if (result.first == SCAN) {
            std::string msg = scanFile(result.second);
            server.SendMessageW(msg);
        }
        else if (result.first == AUTO) {
            extractHourMinutePath(result.second);
            getFile getfile;
            if (!getfile.folderIsExists(pathAuto)) {
                server.SendMessageW("No such folder exists!!!!");
            }
            else {
                server.SendMessageW("This folder exists");
                flag.store(true);
            }
        }
        else if (result.first == HISTORY) {
            server.SendMessageW(history.getData());
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred in management function: " << e.what() << std::endl;
    }
}

void sendHello(Server& server) {
    server.SendMessage("Hello from server!");
}
std::pair<int, std::string> extractRequestAndInfo(const std::string& input) {
    // Extract the request number
    int requestNumber;
    if (input.size() < 2 || !isdigit(input[0]) || !isdigit(input[1])) {
        throw std::runtime_error("Invalid request format");
    }
    requestNumber = (input[0] - '0') * 10 + (input[1] - '0');

    // Extract the rest of the information
    std::string information = input.substr(2);

    return std::make_pair(requestNumber, information);
}


std::queue<std::string> printQueue(std::queue<std::filesystem::path>& q) {
    calculatMD5 getMD5;
    // Create a temporary queue to preserve the original queue
    std::queue<std::filesystem::path> temp = q;
    std::queue<std::string> listFile;


    std::cout << "Queue: ";
    // Dequeue elements and print them until the queue is empty
    while (!temp.empty()) {
        DWORD md5 = getMD5.createMd5(temp.front());

        getMD5.checkHashAndAddPath(md5, temp.front());

        std::cout << md5 << " " << std::endl;
        std::string mess = getMD5.getListFile();
        if (mess.compare("its fine") != 0)
        {
            listFile.push(mess);
        }

        temp.pop(); // Remove the front element
    }
    std::cout << std::endl;
    return listFile;
}

std::string queueToString(std::queue<std::string>& q) {
    std::string result;

    // Iterate over the queue
    std::queue<std::string> temp = q;
    while (!temp.empty()) {
        // Append the current element to the result string
        result += temp.front();

        // Add comma if not the last element
        if (temp.size() > 1) {
            result += ",";
        }

        // Remove the front element
        temp.pop();
    }

    return result;
}

std::string scanFile(std::string path)
{
    getFile getfile;

    std::string errMess = getfile.takeFile(path);
    if (errMess == "No such folder exists!!!!")
    {
        return errMess;
    }
    else
    {
        HistoryDatabase history;
        std::string mess = "";
        std::queue<std::filesystem::path> listFile = getfile.getQueue();
        std::cout << "scan" << std::endl;
        std::queue<std::string> stringListFile = printQueue(listFile);
        if (stringListFile.empty())
        {
            std::cout << "its fine" << std::endl;
            mess = "The files are fine";
        }
        else
        {
            std::cout << "not good" << std::endl;
            std::string mess = queueToString(stringListFile);
            std::cout << "mess: " + mess << std::endl;
            mess = "The suspicious files are: " + mess;
        }
        history.insertData(getCurrentDate(), getCurrentTime(), path, mess);
        return mess;
    }
}

void scheduleFunction( std::atomic<bool>& flag, Server& server) {
    while (true) {
        // Get the current time
        auto now = std::chrono::system_clock::now();
        time_t currentTime = std::chrono::system_clock::to_time_t(now);
        tm localTime;
        localtime_s(&localTime, &currentTime); // Use localtime_s instead of localtime
        // Check if the flag is true and if the current time matches the desired time
        if (flag.load() && localTime.tm_hour == scheduledHour && localTime.tm_min == scheduledMinute) {
            // Call your function
            std::string msg = scanFile(pathAuto);
            //server.SendMessageW(msg);
            std::cout << "heelo its time to go?" << std::endl;

            // Reset the flag back to false until the next scheduled time
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }

        // Sleep for some time before checking again (e.g., every minute)
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}


/*
void extractTime(std::string t)
{
    size_t pos = t.find(':');

    // Extract hour and minute substrings
    std::string hourStr = t.substr(0, pos);
    std::string minuteStr = t.substr(pos + 1);

    // Convert hour and minute substrings to integers and update the global variables
    scheduledHour = std::stoi(hourStr);
    scheduledMinute = std::stoi(minuteStr);
}
*/
/*
void extractHourMinutePath(const std::string& str) {
    // Find the positions of ':' and ','
    size_t colonPos = str.find(':');
    size_t commaPos = str.find(',');

    // Extract hour substring
    scheduledHour = stoi(str.substr(1, colonPos - 1));

    // Extract minute substring
    scheduledMinute = stoi(str.substr(colonPos + 1, commaPos - colonPos - 1));

    // Extract path substring
    pathAuto =  str.substr(commaPos + 1, str.size() - commaPos - 2);

}

void checkUSB() {
    std::map<char, std::set<DWORD>> driveSerialNumbers;
    std::set<char> checkedDrives;
    try
    {
        while (true) {
            _setmode(_fileno(stdout), _O_U16TEXT); // Switch to wide character output mode

            DWORD drives = GetLogicalDrives();
            char driveLetter = 'A';
            std::set<char> connectedDrives;

            while (drives) {
                if (drives & 1) {
                    std::string drivePath = std::string(1, driveLetter) + ":\\";
                    if (isRemovableDriveConnectedViaUSB(drivePath)) {
                        DWORD driveSerialNumber = 0;
                        DWORD maximumComponentLength = 0;
                        DWORD fileSystemFlags = 0;
                        char fileSystemName[MAX_PATH + 1] = { 0 };
                        if (GetVolumeInformationA(drivePath.c_str(), NULL, 0, &driveSerialNumber, &maximumComponentLength, &fileSystemFlags, fileSystemName, MAX_PATH)) {
                            connectedDrives.insert(driveLetter);
                            // If the drive has not been checked while connected before
                            if (checkedDrives.find(driveLetter) == checkedDrives.end()) {
                                driveSerialNumbers[driveLetter] = std::set<DWORD>{ driveSerialNumber };
                                std::wcout << L"Drive " << driveLetter << L" is an external drive connected via USB." << std::endl;
                                std::vector<fs::path> filesOnDrive = listFilesOnDrive(drivePath);
                                std::wcout << L"Files on drive " << driveLetter << L":" << std::endl;
                                if (filesOnDrive.empty()) {
                                    std::wcout << L"No files found." << std::endl;
                                }
                                else {
                                    for (const auto& file : filesOnDrive) {
                                        std::wcout << file.wstring() << std::endl;
                                    }
                                }
                                // Mark the drive as checked
                                checkedDrives.insert(driveLetter);
                            }
                        }
                    }
                    else {
                        if (driveSerialNumbers.find(driveLetter) != driveSerialNumbers.end()) {
                            // If the drive was checked before and now disconnected, remove it from checked drives
                            driveSerialNumbers.erase(driveLetter);
                        }
                    }
                }

                drives >>= 1;
                ++driveLetter;
            }

            // Check if any drives were previously checked but now disconnected and restored
            for (auto it = checkedDrives.begin(); it != checkedDrives.end();) {
                char drive = *it;
                if (connectedDrives.find(drive) == connectedDrives.end()) {
                    it = checkedDrives.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Restore to normal output mode

            // Wait for some time before checking again
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    catch (const std::exception& e) {
        std::wcerr << L"Exception occurred in USB checking: " << e.what() << std::endl;
    }
}



// Function to get the current date
std::string getCurrentDate() {
    std::time_t currentTime = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &currentTime); // Use localtime_s instead of localtime

    char buffer[11]; // Buffer to hold the formatted date (e.g., "YYYY-MM-DD\0")
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &localTime);

    return std::string(buffer);
}

// Function to get the current time
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &nowTime); // Use localtime_s instead of localtime

    char buffer[9]; // Buffer to hold the formatted time (e.g., "HH:MM:SS\0")
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);

    return std::string(buffer);
}


// Function to list all files and directories on the external drive
std::vector<fs::path> listFilesOnDrive(const std::string& drive) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(drive)) {
        files.push_back(entry.path());
    }
    return files;
}



// Function to check if a drive is removable and connected via USB
bool isRemovableDriveConnectedViaUSB(const std::string& drivePath) {
    auto driveType = GetDriveTypeA(drivePath.c_str());
    if (driveType == DRIVE_REMOVABLE) {
        DWORD driveSerialNumber = 0;
        DWORD maximumComponentLength = 0;
        DWORD fileSystemFlags = 0;
        char fileSystemName[MAX_PATH + 1] = { 0 };
        if (GetVolumeInformationA(drivePath.c_str(), NULL, 0, &driveSerialNumber, &maximumComponentLength, &fileSystemFlags, fileSystemName, MAX_PATH)) {
            return (fileSystemFlags & FILE_REMOVABLE_MEDIA) != 0;
        }
    }
    return false;
}
*/