#pragma once

#include <string>
#include <arpa/inet.h>

class Client {
private:
    std::string username;
    sockaddr_in server;
    int client;

    std::string WriteMessage();
    std::string ValidateName();
    std::string ColorMessage();
    bool ValidateMessage(const std::string& message);
    void ReceiveMessage(int client);
    void Registrate();
    void PrintHelpMenu();
public:
    Client();
    ~Client();

    void StartClient();
};
