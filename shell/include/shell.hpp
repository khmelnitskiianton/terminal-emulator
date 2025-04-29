#pragma once

#include <string>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <memory>
#include <cstring>
#include <sys/wait.h>

class Shell
{
    enum CommandNumber
    {
        NONE = 0,
        CD   = 1,
        EXIT = 2,
        NUMBER_COMMANDS
    };

    struct CommandInfo
    {
        CommandNumber commandNum;
        const char* name;
    };

    constexpr static CommandInfo COMMANDS[NUMBER_COMMANDS]
    {
        {NONE},
        {CD, "cd"},
        {EXIT, "exit"}
    };

    class InternalCommand
    {
     public:
        virtual void execute(const std::vector<char*>& argv) = 0;
    };

    class CDcommand : public InternalCommand
    {
     public:
        void execute(const std::vector<char*>& argv) override
        {
            if (argv.at(1) == nullptr) // cd without arguments => move to home directory
            {
                if (chdir(getenv("HOME")) < 0)
                    throw std::runtime_error("Error moving to home directory.");
            }
            else
            {
                if (chdir(argv.at(1)) < 0)
                    throw std::runtime_error("Error changing directory.");
            }
        }
    };

    class ExitCommand : public InternalCommand
    {
     public:
        void execute(const std::vector<char*>& argv) override
        {
            exit(EXIT_SUCCESS);
        }
    };

    std::string currentLine;
    std::vector<std::string> words;
    size_t numberPrograms = 0;
    std::vector<std::vector<char*>> argv;

    static std::unique_ptr<InternalCommand> getCommandObject(CommandNumber commandNumber)
    {
        switch (commandNumber)
        {
            case CD:
                return std::unique_ptr<CDcommand> (new CDcommand());
            case EXIT:
                return std::unique_ptr<ExitCommand> (new ExitCommand());
            default:
                return nullptr;
        }
    }

    static CommandNumber internalCommand(const std::vector<char*>& command)
    {
        char *commandName = command.at(0);
        for (size_t commandNumber = 1; commandNumber < NUMBER_COMMANDS; commandNumber++)
            if (strcmp(commandName, COMMANDS[commandNumber].name) == 0)
                return COMMANDS[commandNumber].commandNum;

        return NONE;
    }

    void clearMem()
    {
        currentLine.clear();
        words.clear();
        argv.clear();
        numberPrograms = 0;
    }

    void splitSentence(const std::string& currentLine)
    {
        std::stringstream ss(currentLine);
        std::string word;

        for (size_t argc = 0; ss >> word; ++argc)
        {
            words.push_back(std::move(word));
        }

        std::vector<char*> firstProgram;
        argv.push_back(std::move(firstProgram));
        for (auto& string : words)
        {
            if (string == "|")
            {
                argv.at(numberPrograms).push_back(nullptr);

                std::vector<char*> nextProgram;
                argv.push_back(std::move(nextProgram));
                ++numberPrograms;
            }
            else
            {
                argv.at(numberPrograms).push_back(string.data());
            }
        }

        argv.at(numberPrograms).push_back(nullptr);
        ++numberPrograms;
    }

    static void closePrevPipes(int pipes[][2], int numberChannels)
    {
        for (int channelNumber = 0; channelNumber < numberChannels; ++channelNumber)
        {
            close(pipes[channelNumber][0]);
            close(pipes[channelNumber][1]);
        }
    }

 public:
    bool processLine()
    {
        if (std::getline(std::cin, currentLine, '\n'))
        {
            splitSentence(currentLine);
            return true;
        }

        return false;
    }

    void executeCommand()
    {
        if (CommandNumber command = internalCommand(argv.at(0)))
        {
            std::unique_ptr<InternalCommand> commandObject = getCommandObject(command);
            if (!commandObject)
                throw std::runtime_error("Failed getting internal command number.");

            commandObject->execute(argv.at(0));
        }
        else
        {
            int pipes[numberPrograms][2];

            for (size_t pipeIndex = 0; pipeIndex < numberPrograms; pipeIndex++)
                pipe(pipes[pipeIndex]);

            pid_t pids[numberPrograms];
            for (size_t programNumber = 0; programNumber < numberPrograms; ++programNumber)
            {
                pid_t pid;

                if ((pid = fork()) < 0)
                {
                    throw std::runtime_error("Forking child process failed.");
                }

                if (pid == 0)
                {
                    if (programNumber > 0)
                        dup2(pipes[programNumber - 1][0], STDIN_FILENO);
                    if (programNumber + 1 < numberPrograms)
                        dup2(pipes[programNumber][1], STDOUT_FILENO);

                    closePrevPipes(pipes, programNumber);

                    if (execvp(argv.at(programNumber).at(0), argv.at(programNumber).data()) < 0)
                        throw std::runtime_error("Child: Exec child process failed.");
                }
                else
                {
                    pids[programNumber] = pid;
                }
            }

            closePrevPipes(pipes, numberPrograms);

            for (size_t processNumber = 0; processNumber < numberPrograms; ++processNumber)
            {
                int status = 0;
                waitpid(pids[processNumber], &status, 0);
                if (status)
                    throw std::runtime_error("Parent: Dead child status.");
            }
        }

        clearMem();
    }
};
