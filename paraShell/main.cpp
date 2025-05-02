#include "ShellConfig.h"
#include "shell.hpp"
#include <iostream>

int main() {
    try {
        Shell shell;
        do {
            std::cerr << "> ";
            shell.execute();
        } while (true);
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
