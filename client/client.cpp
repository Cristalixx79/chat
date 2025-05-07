#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

void receive_messages(int client) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received = recv(client , buffer , sizeof(buffer), 0 );

        if(bytes_received <= 0){
            std::cout << "\033[91m -- Сервер отключился\033[0m\n";
            break;
        }

        std::cout << "\033[90m" << buffer << "\033[0m\n";
    }
}

std::string WriteMessage() {
    std::string message = "";
    std::cout << "\033[90m";
    std::getline(std::cin, message);
    std::cout << "\033[0m";

    return message;
}

int main() {
    std::system("clear");

    int client = socket(AF_INET , SOCK_STREAM , 0 );

    if (client == -1) {
        std::cerr << "\033[91mНе удалось создать сокет\033[0m\n";
        return 1;
    }

    sockaddr_in server{};

    server.sin_addr.s_addr = inet_addr("192.168.2.54");
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);

    if (connect(client , (sockaddr *)&server , sizeof(server)) < 0) {
        std::cerr << "\033[91m -- Не удалось подключиться к серверу\033[0m\n";
        close(client);
        return 1;
    }

    {
        std::cout << "\033[92m -- Подключено к серверу!\n -- Ведите ваше имя: \033[0m";
        std::string username = WriteMessage();
        std::cout << '\n';

        send(client, username.c_str(), username.size(), 0);
        bool isRegistrated = false;

        while (!isRegistrated) {
            char registrationResult[4]{};
            recv(client, registrationResult, sizeof(registrationResult), 0);
            if (std::string(registrationResult) == "200") isRegistrated = true;
            else {
                std::cout << "\033[91m -- Имя уже занято, ведите заново: \033[0m";
                std::string username = WriteMessage();
                std::cout << '\n';

                send(client, username.c_str(), username.size(), 0);
            }
        }

        if(username == "exit()") return 1;
    }

    std::thread(receive_messages, client).detach();

    while(true) {
        std::string message = WriteMessage();

        if(message == "exit()") break;

        send(client , message.c_str() , message.size() , 0 );
    }

    std::cout << "\n\033[93m -- Отключение...\033[0m\n";
    close(client);
    return 0;
}
