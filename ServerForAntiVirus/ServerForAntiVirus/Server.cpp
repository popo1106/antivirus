#include "Server.h"

static const unsigned int IFACE = INADDR_ANY;
static const unsigned short PORT = 1336;




Server::Server() {
    if (!Initialize()) {
        throw std::runtime_error("Failed to initialize Winsock");
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        int error = WSAGetLastError();
        WSACleanup(); // Clean up Winsock if socket creation fails
        throw std::runtime_error(std::string("Failed to create socket: ") + std::to_string(error));
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error(std::string("Bind failed with error: ") + std::to_string(error));
    }
}

bool Server::Initialize() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;
}
void Server::startHandleRequests() {
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error(std::string("Listen failed with error: ") + std::to_string(error));
    }

    std::cout << "Listening for incoming connections..." << std::endl;

    Accept();
}

void Server::Accept() {
    clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        int error = WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error(std::string("Accept failed with error: ") + std::to_string(error));
    }

    std::cout << "Client connected." << std::endl;
}

std::string Server::receiveInformation() {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAENOTSOCK) {
            // Client socket is no longer valid, reset it
            clientSocket = INVALID_SOCKET;
            return ""; // Return empty string to indicate client disconnection
        }
        else {
            // Other error occurred
            closesocket(clientSocket);
            throw std::runtime_error(std::string("Receive failed with error: ") + std::to_string(error));
        }
    }
    else if (bytesReceived == 0) { // Connection closed by client
        closesocket(clientSocket);
        throw std::runtime_error("Connection closed by client.");
    }
    buffer[bytesReceived] = '\0';
    return std::string(buffer);
}


void Server::SendMessage(const std::string& message) {
    int result = send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        closesocket(clientSocket);
        throw std::runtime_error(std::string("Send failed with error: ") + std::to_string(error));
    }
}
void Server::CloseClientSocket() {
    closesocket(clientSocket);
    // You might also want to reset clientSocket to INVALID_SOCKET or perform any necessary cleanup
}
void Server::Close() {
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}
