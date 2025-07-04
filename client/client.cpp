#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "client.h"

namespace {
const int kMaxMessageSize = 1024;
const std::vector<std::string> kLocalCommandList({"/exit", "/help"});
const std::vector<std::string> kPublicCommandList({"/msg"});
}

bool Client::ValidateMessage(const std::string& message) {
    if (message[0] == '/' && message.find(' ') != std::string::npos) {
        if (message.find("-") == std::string::npos) return false;
        std::string command = message.substr(0, message.find(' ')).substr(0, message.find("-"));
        for (auto i : kPublicCommandList) {
            if (i == command) return true;
        }
        return false;
    } else if (message[0] == '/' && message.find(' ') == std::string::npos) {
        for (auto i : kLocalCommandList) {
            if (i == message) return true;
        }
        return false;
    } else return true;
}

void Client::ReceiveMessage(int client) {
    char buffer[kMaxMessageSize]{};
    while (true) {
        std::memset(buffer, 0, kMaxMessageSize);
        ssize_t bytes_received = recv(client, buffer, sizeof(buffer), 0);
        if (std::string(buffer) == "/ban") {
            std::cout << "\033[91m -- You have been permanently banned!\033[0m\n";
            break;
        }
        if (bytes_received <= 0){
            std::cout << "\033[91m -- Сервер отключился\033[0m\n";
            break;
        }
        std::cout << "\033[90m" << buffer << "\033[0m\n";
    }
    close(client);
    std::exit(EXIT_FAILURE);
}

void Client::Registrate() {
    std::cout << "\033[92m -- Подключено к серверу!\n -- Ведите ваше имя: \033[0m";
    username = ValidateName();
    send(client, username.c_str(), username.size(), 0);
    bool isRegistrated = false;
    while (!isRegistrated) {
        char registrationResult[3]{};
        recv(client, registrationResult, sizeof(registrationResult), 0);
        if (std::string(registrationResult) == "200") isRegistrated = true;
        else {
            std::cout << "\033[91m -- Имя уже занято, ведите заново: \033[0m";
            std::string username = ValidateName();
            send(client, username.c_str(), username.size(), 0);
        }
    }
    if(username == "/exit") std::exit(EXIT_SUCCESS);
}

void Client::PrintHelpMenu() {
    std::cout << "\n -- Доступные команды -- \n/msg-\"имя пользователя\" - отправить личное сообщение\n/exit - выйти из программы\n\n";
}

std::string Client::WriteMessage() {
    std::string message = ColorMessage();
    if (ValidateMessage(message)) return message;
    std::cout << "\033[91m -- Такой команды нет, попробуйте снова: \033[0m";
    bool isValidated = false;
    while (!isValidated) {
        message = ColorMessage();
        if (ValidateMessage(message)) isValidated = true;
        else std::cout << "\033[91m -- Такой команды нет, попробуйте снова: \033[0m";
    }
    return message;
}

std::string Client::ValidateName() {
    bool isValidated = false;
    std::string name = "";
    while (!isValidated) {
        name = ColorMessage();
        if (name.find(' ') == std::string::npos && name[0] != '/') isValidated = true;
        else std::cout << "\033[91m -- Имя должно состоять из одного слова: \033[0m";
    }
    return name;
}

std::string Client::ColorMessage() {
    std::string message = "";
    std::cout << "\033[90m";
    std::getline(std::cin, message);
    std::cout << "\033[0m";
    return message;
}

Client::Client() {
    client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1) {
        std::cerr << "\033[91mНе удалось создать сокет\033[0m\n";
        std::exit(EXIT_FAILURE);
    }
    sockaddr_in server = {};
    server.sin_addr.s_addr = inet_addr("192.168.2.70");
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);
    if (connect(client , (sockaddr *)&server , sizeof(server)) < 0) {
        std::cerr << "\033[91m -- Не удалось подключиться к серверу\033[0m\n";
        close(client);
        std::exit(EXIT_FAILURE);
    }
    Registrate();
}

Client::~Client() {
    std::cout << "\n\033[93m -- Отключение...\033[0m\n";
    close(client);
}

void Client::StartClient() {
    std::cout << "\033[92m -- Добро пожаловать в чат, " << username << "!\033[0m\n\n";
    std::thread th(&Client::ReceiveMessage, this, client);
    th.detach();
    while(true) {
        std::string message = WriteMessage();
        if (message == "/exit") break;
        if (message == "/help") PrintHelpMenu();
        else send(client, message.c_str(), message.size(), 0);
    }
}

int main() {
    std::system("clear");

    Client cl;
    cl.StartClient();

    return 0;
}
