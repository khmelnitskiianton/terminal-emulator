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
    enum ShellError
    {
        ERROR_OK = 0,
        ERROR_GET_INTERNAL = 1,
        ERROR_EXEC_CHILD = 2,
        ERROR_STATUS_CHILD = 3,
        ERROR_CHANGE_DIR = 4,
        ERROR_MOVE_HOME = 5,
        ERROR_FORK = 6
    };

    const char *get_error_msg(ShellError error)
    {
        switch (error)
        {
            case ERROR_OK:
                return "No error.";
            case ERROR_GET_INTERNAL:
                return "Error allocating internal command object.";
            case ERROR_EXEC_CHILD:
                return "Execvp return error.";
            case ERROR_STATUS_CHILD:
                return "Child return error.";
            case ERROR_CHANGE_DIR:
                return "CD change directory error.";
            case ERROR_MOVE_HOME:
                return "Error cd without args (move to home directory).";
            case ERROR_FORK:
                return "Fork error.";
            default:
                return "Unknow error type.";
        }
    }

    void print_error(ShellError error)
    {
        std::cerr << get_error_msg(error) << std::endl;
    }

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
        virtual ShellError execute(const std::vector<char*>& argv) = 0;
    };

    class CDcommand : public InternalCommand
    {
     public:
        ShellError execute(const std::vector<char*>& argv) override
        {
            if (argv.at(1) == nullptr) // cd without arguments => move to home directory
            {
                if (chdir(getenv("HOME")) < 0)
                    return ERROR_MOVE_HOME;
            }
            else
            {
                if (chdir(argv.at(1)) < 0)
                    return ERROR_CHANGE_DIR;
            }

            return ERROR_OK;
        }
    };

    class ExitCommand : public InternalCommand
    {
     public:
        ShellError execute(const std::vector<char*>& argv) override
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

    ShellError executeExternalCommands()
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
                return ERROR_FORK;
            }

            if (pid == 0)
            {
                if (programNumber > 0)
                    dup2(pipes[programNumber - 1][0], STDIN_FILENO);
                if (programNumber + 1 < numberPrograms)
                    dup2(pipes[programNumber][1], STDOUT_FILENO);

                closePrevPipes(pipes, programNumber);

                if (execvp(argv.at(programNumber).at(0), argv.at(programNumber).data()) < 0)
                    return ERROR_EXEC_CHILD;
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
                return ERROR_STATUS_CHILD;
        }

        return ERROR_OK;
    }

    static void closePrevPipes(int pipes[][2], int numberChannels)
    {
        for (int channelNumber = 0; channelNumber < numberChannels; ++channelNumber)
        {
            close(pipes[channelNumber][0]);
            close(pipes[channelNumber][1]);
        }
    }

    bool processLine()
    {
        if (std::getline(std::cin, currentLine, '\n') && currentLine.size())
        {
            splitSentence(currentLine);
            return true;
        }

        return false;
    }

    ShellError executeCommand()
    {
        ShellError error = ERROR_OK;
        if (CommandNumber command = internalCommand(argv.at(0)))
        {
            std::unique_ptr<InternalCommand> commandObject = getCommandObject(command);
            if (!commandObject)
                return ERROR_GET_INTERNAL;

            error = commandObject->execute(argv.at(0));
        }
        else
        {
            error = executeExternalCommands();
        }

        return error;
    }

 public:
    void execute()
    {
        if (!processLine())
            return;

        ShellError error = executeCommand();
        if (error)
        {
            print_error(error);
        }

        clearMem();
    }
};
