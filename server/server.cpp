
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "server.h"

int CountChar(char ch[]) {
    int i = 0;
    while (ch[i] != '\0') i++;
    return i;
}

namespace {
const int kMaxBacklog = 10;
const int kMaxUsernameSize = 32;
const int kMaxMessageSize = 1024;
const std::string kRegistrationSuccsses = "200";
const std::string kRegistrationError = "300";
const std::string kPrivateMessageSendingError = "\033[91mServer: error while sending private message. User not found\033[0m";
const std::string kWarningCommand = "\033[91mWarning! Don't spam! Next warning = ban!\033[0m";
const std::string kBanCommand = "/ban";
const std::vector<std::string> adminCommands({"/warn", "/ban"});
}

void Server::AdminPanel() {
    while (true) {
        std::string action = "";
        std::getline(std::cin, action);
        std::string command = action.substr(0, action.find(' '));
        std::string user = action.substr(action.find(' ') + 1, action.size() - 1);

        if (action.find(' ') == std::string::npos) {
            std::cout << "\033[91m -- Неправильный формат команды \033[0m\n";
            continue;
        }
        if (!IsCommandFound(command)) {
            std::cout << "\033[91m -- Команда \"" << command << "\" не найдена. Попробуйте снова: \033[0m";
            continue;
        }
        if (!IsUserFound(user)) {
            std::cout << "\033[91m -- Пользователь \"" << user << "\" не найден. Попробуйте снова: \033[0m";
            continue;
        }

        if (command == "/warn") {
            std::lock_guard<std::mutex> lock(clientMutex);
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == user) {
                    send((*it).first, kWarningCommand.c_str(), kWarningCommand.size(), 0);
                    break;
                }
            }
        } else if (command == "/ban") {
            std::lock_guard<std::mutex> lock(clientMutex);
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == user) {
                    send((*it).first, kBanCommand.c_str(), kBanCommand.size(), 0);
                    break;
                }
            }
        }
    }
}

void Server::ClientHandle(int clientSocket) {
    char username[kMaxUsernameSize]{};
    if (!IsRegistrated(clientSocket, username)) return;
    counter++;
    std::cout << "\033[92m -- Пользователь " << username << " подключился! Всего пользователей: " << counter << "\033[0m\n";
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        users.push_back(std::pair<int, std::string>(clientSocket, username));
    }
    char buffer[kMaxMessageSize]{};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t clientBytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (clientBytesReceived <= 0) {
            std::cout << "\033[91m -- Пользователь " << username << " отключился! Всего пользователей: " << (users.size() - 1) << "\033[0m\n";
            counter--;
            break;
        }
        std::string message(username, CountChar(username));
        message = message + ": " + std::string(buffer);

        if (message.find("/msg-") != std::string::npos) {
            std::string temp = message.erase(0, message.find("/msg-") + 5);
            std::string address = temp.erase(temp.find(' '), std::string::npos);
            bool isSent = false;
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == address) {
                    std::string privateMessage = std::string(username) + "(private):" + message.substr(message.find(' '), message.size());
                    SendPrivateMessage(privateMessage, (*it).first);
                    isSent = true;
                    break;
                }
            }
            if (!isSent) send(clientSocket, kPrivateMessageSendingError.c_str(), kPrivateMessageSendingError.size(), 0);
        } else {
            std::cout << "\033[90m >> Отправлено пользователем " << message << "\033[0m\n";
            SendBroadcastMessage(message, clientSocket);
        }
    }

    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (auto it = users.begin(); it != users.end(); it++) {
            if ((*it).first == clientSocket && (*it).second == username) {
                users.erase(it);
                break;
            }
        }
    }
}

void Server::SendBroadcastMessage(const std::string& message, const int receiverSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (auto it : users) {
        if (it.first != receiverSocket) {
            send(it.first, message.c_str(), message.size(), 0);
        }
    }
}

void Server::SendPrivateMessage(const std::string& message, const int receiverSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (auto it : users) {
        if (it.first == receiverSocket) {
            send(it.first, message.c_str(), message.size(), 0);
            break;
        }
    }
}

bool Server::IsRegistrated(int clientSocket, char* username) {
    bool isRegistrated = false;
    ssize_t clientBytesReceived = 0;
    while (!isRegistrated) {
        memset(username, 0, kMaxUsernameSize);
        clientBytesReceived = recv(clientSocket, username, sizeof(username), 0);
        if (clientBytesReceived <= 0) {
            std::cout << "\033[91m -- Пользователь " << username << " отключился! Всего пользователей: " << (users.size() - 1) << "\033[0m\n";
            counter--;
            break;
        }
        if (users.size() == 0) {
            isRegistrated = true;
        } else {
            for (size_t i = 0; i < users.size(); i++) {
                if (std::string(username) == users[i].second) {
                    isRegistrated = false;
                    break;
                } else {
                    isRegistrated = true;
                }
            }
        }
        if (isRegistrated) {
            send(clientSocket, kRegistrationSuccsses.c_str(), kRegistrationSuccsses.size(), 0);
            return true;
        } else {
            send(clientSocket, kRegistrationError.c_str(), kRegistrationError.size(), 0);
        }
    }
    return false;
}

bool Server::IsUserFound(const std::string& user) {
    for (auto it = users.begin(); it != users.end(); it++) {
        if ((*it).second == user) {
            return true;
        }
    }
    return false;
}

bool Server::IsCommandFound(const std::string& command) {
    for (auto i : adminCommands) {
        if (command == i) return true;
    }
    return false;
}

Server::Server() {
    counter = 0;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "\033[91m -- Ошибка создания сокета\033[0m\n";
        std::exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(54000);
    if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "\033[91m -- Ошибка bind\033[0m\n";
        close(serverSocket);
        std::exit(EXIT_FAILURE);
    }
    if (listen(serverSocket, kMaxBacklog) < 0) {
        std::cerr << "\033[91m -- Ошибка listen\033[0m\n";
        close(serverSocket);
        std::exit(EXIT_FAILURE);
    }
    std::cout << "\033[90m -- Ожидение пользователей...\033[0m\n";
}

Server::~Server() {
    close(serverSocket);
}

void Server::StartServer() {
    std::thread th(&Server::AdminPanel, this);
    th.detach();
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressLen);
        if (clientSocket < 0) {
            std::cerr << "\033[91m -- Ошибка accept\033[0m\n";
            continue;
        }
        std::thread th(&Server::ClientHandle, this, clientSocket);
        th.detach();
    }
}

int main() {
    std::system("clear");

    Server server;
    server.StartServer();

    return 0;
}
