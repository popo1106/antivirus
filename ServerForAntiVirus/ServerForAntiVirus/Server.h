#pragma once
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <winsock2.h> // Include Winsock2 header for socket programming
#include <io.h>       // Include for _setmode
#include <fcntl.h>  

#define SCAN 1
#define AUTO 2
#define HISTORY 3

class Server {
public:
    Server();
    ~Server() {};
    bool Initialize();
    //bool bindAndListen();
    //bool Listen();
    void Accept();
    void Close();
    void CloseClientSocket();
    void startHandleRequests();
    void SendMessage(const std::string& message);

    std::string receiveInformation();


private:
    SOCKET serverSocket;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;
    int clientAddrLen;
};
