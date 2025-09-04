#include "sender.h"

void Sender::SendBroadcast(userList users, const std::string& msg, int recevierSocket) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto it : users) {
        if (it.first != recevierSocket) {
            send(it.first, msg.c_str(), msg.size(), 0);
        }
    }
}

void Sender::SendPrivate(userList users, const std::string& msg, int recevierSocket) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto it : users) {
        if (it.first == recevierSocket) {
            send(it.first, msg.c_str(), msg.size(), 0);
            break;
        }
    }
}