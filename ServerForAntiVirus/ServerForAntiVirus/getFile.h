#include <iostream>
#include <filesystem>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>

#define NUM_THREADS 10
class getFile {
public:
    getFile() {};
    ~getFile() {};

    void processFile(const std::filesystem::directory_entry& entry);
    void processDirectory(const std::string& directoryPath);
    //std::string takeFile(std::string& userDirectory);
    std::string takeFile(const std::string& userDirectory);
    std::queue<std::filesystem::path> getQueue();
    //bool folderIsExists(std::string userDirectory);
    bool folderIsExists(const std::string& userDirectory);

private:
    std::mutex printMutex;
    std::queue<std::filesystem::path> myQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    bool stopThreads = false;
    const size_t maxThreads = std::thread::hardware_concurrency(); // Limit threads to hardware concurrency
    std::vector<std::thread> threadPool;
    void threadWorker();
};
