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
}

std::vector<std::pair<int, std::string>> users;
std::mutex clients_mutex;

int counter = 0;

void SendBroadcastMessage(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it : users) {
        if (it.first != sender_socket) {
            send(it.first, message.c_str(), message.size(), 0);
        }
    }
}
void SendPrivateMessage(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto it : users) {
        if (it.first == sender_socket) {
            send(it.first, message.c_str(), message.size(), 0);
            break;
        }
    }
}

void ClientHandle(int client_socket) {
    char buffer[kMaxMessageSize]{};
    char username[kMaxUsernameSize]{};
    bool isRegistrated = false;
    ssize_t username_bytes_received = 0;

    while (!isRegistrated) {
        memset(username, 0, sizeof(username));
        ssize_t username_bytes_received = recv(client_socket, username, sizeof(username), 0);

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
            send(client_socket, registrationSuccsses.c_str(), registrationSuccsses.size(), 0);
        } else {
            send(client_socket, registrationError.c_str(), registrationError.size(), 0);
        }
    }
    counter++;
    std::cout << "\033[92m -- Пользователь " << username << " подключился! Всего пользователей: " << counter << "\033[0m\n";

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        users.push_back(std::pair(client_socket, username));
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
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

            for (auto it = users.begin(); it != users.end(); it++) {
                if ((*it).second == address) {
                    std::string privateMessage = std::string(username) + "(private):" + message.substr(message.find(" "), message.size());
                    SendPrivateMessage(privateMessage, (*it).first);
                    message = "";
                    break;
                }
            }
        } else {
            std::cout << "\033[90m >> Отправлено пользователем " << message << "\033[0m\n";
            SendBroadcastMessage(message, client_socket);
            message = "";
        }
    }

    close(client_socket);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto it = users.begin(); it != users.end(); it++) {
            if ((*it).first == client_socket && (*it).second == username) {
                users.erase(it);
                break;
            }
        }
    }
}

int main() {
    std::system("clear");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "\033[91m -- Ошибка создания сокета\033[0m\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // слушать все интерфейсы
    address.sin_port = htons(54000);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "\033[91m -- Ошибка bind\033[0m\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, kMaxBacklog) < 0) {
        std::cerr << "\033[91m -- Ошибка listen\033[0m\n";
        close(server_fd);
        return 1;
    }

    std::cout << "\033[90m -- Ожидение пользователей...\033[0m\n";

    while (true) {
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);

        int client_socket = accept(server_fd, (sockaddr*)&client_address, &client_len);

        if (client_socket < 0) {
            std::cerr << "\033[91m -- Ошибка accept\033[0m\n";
            continue;
        }

        std::thread(ClientHandle, client_socket).detach();
    }

    close(server_fd);
    return 0;
}
