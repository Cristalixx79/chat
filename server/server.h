#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "sender.h"

typedef std::vector<std::pair<int, std::string>> userList;

class Server {
private:
    userList users;
    std::mutex clientMutex;
    sockaddr_in address;
    Sender sender;

    int serverSocket;
    int counter;

    void AdminPanel();
    void ClientHandle(int clientSocket);
    bool IsRegistrated(int clientSocket, char* username);
    bool IsUserFound(const std::string& user);
    bool IsCommandFound(const std::string& command);
public:
    Server();
    ~Server();

    void StartServer();
};
