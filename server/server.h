#pragma once

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <netinet/in.h>

class Server {
private:
    std::vector<std::pair<int, std::string>> users;
    std::mutex clientMutex;
    sockaddr_in address;
    int serverSocket;
    int counter;

    void AdminPanel();
    void ClientHandle(int clientSocket);
    void SendBroadcastMessage(const std::string& message, const int receiverSocket);
    void SendPrivateMessage(const std::string& message, const int receiverSocket);
    bool IsRegistrated(int clientSocket, char* username);
    bool IsUserFound(const std::string& user);
    bool IsCommandFound(const std::string& command);
public:
    Server();
    ~Server();

    void StartServer();
};
