#include "adminPanel.h"

std::vector<std::string> AdminPanel::GetCommandData()
{
    std::vector<std::string> commandData;
    std::string cmd;
    std::getline(std::cin, cmd);

    while (!CheckCommandInput(cmd))
    {
        std::getline(std::cin, cmd);
    }

    if (cmd.find(' ') != std::string::npos)
    {
        commandData.push_back(cmd.substr(0, cmd.find(' ')));
        commandData.push_back(cmd.substr(cmd.find(' ') + 1, cmd.size()));
    }
    else
    {
        commandData.push_back(cmd);
        commandData.push_back("\0");
    }

    return commandData;
}

bool AdminPanel::CheckCommandInput(const std::string &cmd)
{
    return (cmd.size() > 1) && (cmd[0] == '/');
}

bool AdminPanel::CheckUser(const std::string& username) {
    for (auto& i : users) {
        if (username == i.second) return true;
    }
    return false;
}

void AdminPanel::RunCommand(std::vector<std::string> &commandData)
{
    if (!CheckUser(commandData[1])) {
        return;
    }

    if (commandData[0] == Admin::Commands::userAddressedCommands[0])
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = users.begin(); it != users.end(); it++)
        {
            if ((*it).second == commandData[1])
            {
                sender.SendPrivate(users, Admin::kWarningCommand.c_str(), (*it).first);
                break;
            }
        }
    }
    else if (commandData[0] == Admin::Commands::userAddressedCommands[1])
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = users.begin(); it != users.end(); it++)
        {
            if ((*it).second == commandData[1])
            {
                sender.SendPrivate(users, Admin::Commands::userAddressedCommands[1].c_str(), (*it).first);
                break;
            }
        }
    }
}

void AdminPanel::Update(const int clientSocket, const std::string &username)
{
    users.push_back(std::pair<int, std::string>(clientSocket, username));
}

void AdminPanel::Init()
{
    while (true)
    {
        std::vector<std::string> commandData = GetCommandData();
        RunCommand(commandData);
    }
}