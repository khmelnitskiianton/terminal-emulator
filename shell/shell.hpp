#pragma once
#include <string>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <sys/wait.h>

class Shell
{
    std::string currentLine;
    std::vector<std::string> words;
    char** argv;

 public:
    bool processLine()
    {
        if (std::getline(std::cin, currentLine, '\n'))
        {
            std::cin >> std::ws;

            splitSentence(currentLine);
            return true;
        }
        return false;
    }

    void splitSentence(const std::string& currentLine)
    {
        std::stringstream ss(currentLine);
        std::string word;
        for (size_t argc = 0; ss >> word; ++argc)
        {
            argv[argc] = word.data();
            words.push_back(std::move(word));
        }
    }

    void executeCommand()
    {
        pid_t pid = 0;;
        int status = 0;

        if ((pid = fork()) < 0)
        {
            throw std::runtime_error ("Forking child process failed.\n");
        }
        else if (pid == 0)
        {
            if (execvp(argv[0], argv) < 0)
                throw std::runtime_error ("Exec child process failed.\n");
        }
        else
        {
            waitpid(pid, &status, 0);
        }

        clearMem();
    }

    void clearMem()
    {
        currentLine.clear();
        words.clear();
    }
};
