#include <iostream>
#include "shell.hpp"

int main()
{
    try
    {
        Shell shell;
        bool status = false;
        do
        {
            status = shell.processLine();
            if (status == false)
                break;

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
