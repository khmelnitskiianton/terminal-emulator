#include <iostream>
#include "ShellConfig.h"
#include "shell.hpp"

int main()
{
    try
    {
        Shell shell;
        bool status = false;
        do
        {
            std::cerr << "> ";
            status = shell.processLine();
            shell.executeCommand();
        } while (status);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
