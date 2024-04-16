#include "getFile.h"
#include <iostream>
#include <vector>
#include <filesystem>
  
bool getFile::folderIsExists(const std::string& userDirectory) {
    // Check if the userDirectory is empty
    if (userDirectory.empty()) {
        return false;
    }

    // Check if the provided path exists and is a directory
    return std::filesystem::exists(userDirectory) ||
        std::filesystem::exists(std::filesystem::path(userDirectory).parent_path());
}

void getFile::processFile(const std::filesystem::directory_entry& entry) {
    std::lock_guard<std::mutex> lock(queueMutex);
    myQueue.push(entry.path());
    std::cout << "Added file to queue: " << entry.path() << std::endl;
}

void getFile::processDirectory(const std::string& directoryPath) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                processFile(entry);
            }
            else if (entry.is_directory()) {
                processDirectory(entry.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error accessing file or directory: " << e.what() << std::endl;
    }
}

std::string getFile::takeFile(const std::string& userDirectory) {
    if (!folderIsExists(userDirectory)) {
        return "No such folder exists!!!!";
    }

    // Run the processDirectory function
    processDirectory(userDirectory);

    return "";
}

std::queue<std::filesystem::path> getFile::getQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return myQueue;
}

/*
void getFile::processFile(const std::filesystem::directory_entry& entry) {
    std::lock_guard<std::mutex> lock(queueMutex);
    myQueue.push(entry.path());
    std::cout << "Added file to queue: " << entry.path() << std::endl;
}

void getFile::processDirectory(const std::string& directoryPath) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                processFile(entry);
            }
            else if (entry.is_directory()) {
                processDirectory(entry.path().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error accessing file or directory: " << e.what() << std::endl;
    }
}

std::string getFile::takeFile(std::string userDirectory) {
    if (!folderIsExists(userDirectory)) {
        return "No such folder exists!!!!";
    }

    // Run the processDirectory function
    processDirectory(userDirectory);

    return "";
}

std::queue<std::filesystem::path> getFile::getQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return myQueue;
}

bool getFile::folderIsExists(std::string userDirectory) {
    do {
        // check if the entered directory is good
        if (!std::filesystem::is_directory(userDirectory)) {
            return false;
        }
    } while (userDirectory.empty());
    return true;
}

/*
#include "getFile.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
void getFile::processFile(const std::filesystem::directory_entry& entry)
{
    // Process file logic here
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        myQueue.push(entry.path());
    }
    queueCondition.notify_one();
}

void getFile::processDirectory(const std::string& directoryPath)
{
    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        try {
            if (entry.is_regular_file())
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                myQueue.push(entry.path());
                queueCondition.notify_one();
            }
            else if (entry.is_directory())
            {
                processDirectory(entry.path().string());
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error accessing file or directory: " << e.what() << std::endl;
        }
    }
}

void getFile::threadWorker()
{
    while (true)
    {
        std::filesystem::path filePath;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this]() { return !myQueue.empty() || stopThreads; });
            if (stopThreads && myQueue.empty())
                return;
            filePath = myQueue.front();
            myQueue.pop();
        }
        // Process file
        // For example: processFile(filePath);
    }
}

std::string getFile::takeFile(std::string userDirectory)
{

    if (!folderIsExists(userDirectory))
    {
        return "No such folder exists!!!!";
    }
    // Start worker threads
    for (size_t i = 0; i < maxThreads; ++i)
    {
        threadPool.emplace_back(&getFile::threadWorker, this);
    }

    // Run the processDirectory function
    processDirectory(userDirectory);

    // Signal worker threads to stop
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopThreads = true;
    }
    queueCondition.notify_all();

    // Join worker threads
    for (auto& thread : threadPool) {
        thread.join();
    }

    return "";
}

std::queue<std::filesystem::path> getFile::getQueue()
{
    return myQueue;
}

bool getFile::folderIsExists(std::string userDirectory)
{
    do {
        // check if the entered directory is good
        if (!std::filesystem::is_directory(userDirectory)) {
            return false;
            // clear the input buffer
            userDirectory.clear();
        }
    } while (userDirectory.empty());
    return true;
}
*/