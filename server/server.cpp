#include "server.h"

void Server::ClientHandle(int clientSocket)
{
    char username[kMaxUsernameSize]{};
    if (!IsRegistrated(clientSocket, username)) return;
    counter++;

    std::cout << "\033[92m -- Пользователь " << username << " подключился! Всего пользователей: " << counter << "\033[0m\n";
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        users.push_back(std::pair<int, std::string>(clientSocket, username));
    }

    char buffer[kMaxMessageSize]{};
    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t clientBytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (clientBytesReceived <= 0)
        {
            std::cout << "\033[91m -- Пользователь " << username << " отключился! Всего пользователей: " << (users.size() - 1) << "\033[0m\n";
            counter--;
            break;
        }
        std::string message(username, std::strlen(username));
        message = message + ": " + std::string(buffer);

        if (message.find("/msg-") != std::string::npos)
        {
            std::string temp = message.erase(0, message.find("/msg-") + 5);
            std::string address = temp.erase(temp.find(' '), std::string::npos);
            bool isSent = false;
            for (auto it = users.begin(); it != users.end(); it++)
            {
                if ((*it).second == address)
                {
                    std::string privateMessage = std::string(username) + "(private):" + message.substr(message.find(' '), message.size());
                    sender.SendPrivate(users, privateMessage, (*it).first);
                    isSent = true;
                    break;
                }
            }
            if (!isSent)
                sender.SendPrivate(users, kPrivateMessageSendingError.c_str(), clientSocket);
        }
        else
        {
            std::cout << "\033[90m >> Отправлено пользователем " << message << "\033[0m\n";
            sender.SendBroadcast(users, message, clientSocket);
        }
    }

    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (auto it = users.begin(); it != users.end(); it++)
        {
            if ((*it).first == clientSocket && (*it).second == username)
            {
                users.erase(it);
                break;
            }
        }
    }
}

bool Server::IsRegistrated(int clientSocket, char *username)
{
    bool isRegistrated = false;
    ssize_t clientBytesReceived = 0;
    while (!isRegistrated)
    {
        std::memset(username, 0, kMaxUsernameSize);
        clientBytesReceived = recv(clientSocket, username, sizeof(username), 0);
        if (clientBytesReceived <= 0)
        {
            std::cout << "\033[91m -- Пользователь " << username << " отключился! Всего пользователей: " << (users.size() - 1) << "\033[0m\n";
            counter--;
            break;
        }
        if (users.size() == 0)
        {
            isRegistrated = true;
        }
        else
        {
            for (size_t i = 0; i < users.size(); i++)
            {
                if (std::string(username) == users[i].second)
                {
                    isRegistrated = false;
                    break;
                }
                else
                {
                    isRegistrated = true;
                }
            }
        }
        if (isRegistrated)
        {
            sender.SendPrivate(users, kRegistrationSuccsses, clientSocket);
            panel.Update(clientSocket, username);
            return true;
        }
        else
        {
            sender.SendPrivate(users, kRegistrationError, clientSocket);
        }
    }
    return false;
}

void Server::StartServer()
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "\033[91m -- Ошибка создания сокета\033[0m\n";
        std::exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(54000);
    if (bind(serverSocket, (sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "\033[91m -- Ошибка bind\033[0m\n";
        close(serverSocket);
        std::exit(EXIT_FAILURE);
    }
    if (listen(serverSocket, kMaxBacklog) < 0)
    {
        std::cerr << "\033[91m -- Ошибка listen\033[0m\n";
        close(serverSocket);
        std::exit(EXIT_FAILURE);
    }

    std::thread th([this]()
                   { panel.Init(); });

    std::cout << "\033[90m -- Ожидение пользователей...\033[0m\n";
    while (true)
    {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket < 0)
        {
            std::cerr << "\033[91m -- Ошибка accept\033[0m\n";
            continue;
        }
        std::thread th2([this, clientSocket]()
                        { this->ClientHandle(clientSocket); });
        th2.detach();
    }
}

int main()
{
    std::system("clear");

    Server server;
    server.StartServer();

    return 0;
}

Server::Server()
{
    counter = 0;
}
Server::~Server()
{
    close(serverSocket);
}