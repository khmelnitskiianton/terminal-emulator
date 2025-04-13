#pragma once
#include <string>
#include <vector>
#include <unistd.h>

class Shell
{
    std::vector<std::string> currentLine;
 public:
    bool processLine()
    {
        std::string line;
        if (std::getline(std::cin, line, ' '))
        {
            std::cin >> std::ws;
            currentLine.emplace_back(std::move(line));
            return true;
        }
        return false;
    }

    bool executeCommand()
    {
        pid_t pid;
        if ((pid = fork()) < 0)
            throw std::exception ("Forking child process failed.\n");
        else if (pid == 0)
        {
            if (execvp(currentLine.at(0).c_str(), ))
    }
};
