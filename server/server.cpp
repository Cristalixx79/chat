#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int CountChar(char ch[]) {
    int i = 0;
    while (ch[i] != '\0') i++;
    return i;
}

namespace {
const int kMaxBacklog = 10;
const int kMaxUsernameSize = 32;
const int kMaxMessageSize = 1024;
const std::string registrationSuccsses = "200";
const std::string registrationError = "300";
const std::string privateMessageSendingError = "\033[91mServer: error while sending private message. User not found\033[0m";
const std::string warning = "\033[91mWarning! Don't spam! Next warning = ban!\033[0m";
const std::string ban = "/ban";
const std::vector<std::string> adminCommands({"/warn", "/ban"});
}

std::vector<std::pair<int, std::string>> users;
std::mutex clientMutex;

int counter = 0;

void SendBroadcastMessage(const std::string& message, int receiverSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (auto it : users) {
        if (it.first != receiverSocket) {
            send(it.first, message.c_str(), message.size(), 0);
        }
    }
}
void SendPrivateMessage(const std::string& message, int receiverSocket, int clientSocket) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (auto it : users) {
        if (it.first == receiverSocket) {
            send(it.first, message.c_str(), message.size(), 0);
            break;
        }
    }
}

void ClientHandle(int clientSocket) {
    char username[kMaxUsernameSize]{};
    bool isRegistrated = false;
    ssize_t clientBytesReceived = 0;

    while (!isRegistrated) {
        memset(username, 0, sizeof(username));
        ssize_t clientBytesReceived = recv(clientSocket, username, sizeof(username), 0);

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
            send(clientSocket, registrationSuccsses.c_str(), registrationSuccsses.size(), 0);
        } else {
            send(clientSocket, registrationError.c_str(), registrationError.size(), 0);
        }
    }
    counter++;
    std::cout << "\033[92m -- Пользователь " << username << " подключился! Всего пользователей: " << counter << "\033[0m\n";

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        users.push_back(std::pair(clientSocket, username));
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
        message.append(": ");
        message.append(buffer);
        message.shrink_to_fit();

        if (message.find("/msg-") != std::string::npos) {
            std::string temp = message.erase(0, message.find("/msg-") + 5);
            std::string address = temp.erase(temp.find(" "), std::string::npos);
            bool isSent = false;
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == address) {
                    std::string privateMessage = std::string(username) + "(private):" + message.substr(message.find(" "), message.size());
                    SendPrivateMessage(privateMessage, (*it).first, clientSocket);
                    isSent = true;
                    break;
                }
            }
            if (!isSent) send(clientSocket, privateMessageSendingError.c_str(), privateMessageSendingError.size(), 0);
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

void AdminPanel() {
    while (true) {
        std::string action = "";
        std::getline(std::cin, action);
        std::string command = action.substr(0, action.find(" "));
        std::string user = action.substr(action.find(" ") + 1, action.size() - 1);

        bool isAvailableCommand = false;
        for (auto i : adminCommands) {
            if (command == i) isAvailableCommand = true;
        }
        if (!isAvailableCommand) {
            std::cout << "\033[91m -- \"" << command << "\" command not found. Try again: \033[0m";
            continue;
        }

        bool isUserFound = false;
        for (auto it = users.begin(); it != users.end(); it++) {
            if ((*it).second == user) {
                isUserFound = true;
                break;
            }
        }
        if (!isUserFound) {
            std::cout << "\033[91m -- \"" << user << "\" user not found. Try again: \033[0m";
            continue;
        }

        if (command == "/warn") {
            std::lock_guard<std::mutex> lock(clientMutex);
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == user) {
                    send((*it).first, warning.c_str(), warning.size(), 0);
                    break;
                }
            }
        } else if (command == "/ban") {
            std::lock_guard<std::mutex> lock(clientMutex);
            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == user) {
                    send((*it).first, ban.c_str(), ban.size(), 0);
                    break;
                }
            }
        }
    }
}

int main() {
    std::system("clear");

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "\033[91m -- Ошибка создания сокета\033[0m\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // слушать все интерфейсы
    address.sin_port = htons(54000);

    if (bind(serverSocket, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "\033[91m -- Ошибка bind\033[0m\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, kMaxBacklog) < 0) {
        std::cerr << "\033[91m -- Ошибка listen\033[0m\n";
        close(serverSocket);
        return 1;
    }

    std::cout << "\033[90m -- Ожидение пользователей...\033[0m\n";
    std::thread(AdminPanel).detach();

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLen = sizeof(clientAddress);

        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressLen);

        if (clientSocket < 0) {
            std::cerr << "\033[91m -- Ошибка accept\033[0m\n";
            continue;
        }

        std::thread(ClientHandle, clientSocket).detach();
    }

    close(serverSocket);
    return 0;
}
