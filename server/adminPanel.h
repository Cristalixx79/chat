#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>

#include "sender.h"

typedef std::vector<std::pair<int, std::string>> userList;

namespace Admin {    
    namespace Commands {
        const std::vector<std::string> userAddressedCommands({"/warn", "/ban"});
    }

    const std::string kWarningMessage = "\033[91mWarning! Don't spam! Next warning = ban!\033[0m";
}

class AdminPanel {
    std::mutex mtx;
    userList users;

    Sender sender;
public:
    void Init();
    void Update(const int clientsocket, const std::string& username);
    std::vector<std::string> GetCommandData();
    bool CheckCommandInput(const std::string& cmd);
    bool CheckUser(const std::string& username);
    void RunCommand(std::vector<std::string>& commandData);
};