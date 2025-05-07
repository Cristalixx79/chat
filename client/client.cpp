#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

void receive_messages(int sock) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        ssize_t bytes_received = recv(sock , buffer , sizeof(buffer), 0 );

        if(bytes_received <= 0){
            std::cout << "Сервер отключился\n";
            break;
        }

        std::cout << buffer << "\n";
    }
}

int main() {
    std::system("clear");

    int sock = socket(AF_INET , SOCK_STREAM , 0 );

    if (sock == -1) {
        std::cerr << "Не удалось создать сокет\n";
        return 1;
    }

    sockaddr_in server{};

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(54000);

    if (connect(sock , (sockaddr *)&server , sizeof(server)) < 0) {
        std::cerr << " -- Не удалось подключиться к серверу\n";
        close(sock);
        return 1;
    }

    {
        std::string username = "";
        std::cout << " -- Подключено к серверу!\n -- Ведите ваше имя: ";
        std::getline(std::cin, username);
        send(sock, username.c_str(), username.size(), 0);
        bool isRegistrated = false;

        while (!isRegistrated) {
            char registrationResult[4]{};
            recv(sock, registrationResult, sizeof(registrationResult), 0);
            if (std::string(registrationResult) == "200") isRegistrated = true;
            else {
                std::cout << " -- Имя уже занято, ведите заново: ";
                username = "";
                std::getline(std::cin, username);
                send(sock, username.c_str(), username.size(), 0);
            }
            registrationResult[4] = {};
        }

        if(username == "exit") return 1;
    }

    std::thread(receive_messages, sock).detach();

    while(true) {
        std::string message;
        std::getline(std::cin, message);
        if(message == "exit") break;

        send(sock , message.c_str() , message.size() , 0 );
    }

   close(sock);

   return 0;
}
