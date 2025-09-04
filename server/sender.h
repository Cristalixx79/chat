#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>

typedef std::vector<std::pair<int, std::string>> userList;

class Sender {
    std::mutex mtx;
public:
    void SendBroadcast(userList users, const std::string& msg, int recevierSocket);
    void SendPrivate(userList users, const std::string& msg, int recevierSocket);
};