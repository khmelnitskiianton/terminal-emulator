#include "shell.hpp"

int main()
{
    Shell shell();
    bool status = false;
    do
    {
        status = shell.processLine();
        status = shell.executeCommand();
    } while (status);

    return 0;
}
