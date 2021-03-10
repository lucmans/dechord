
#include "config.h"

#include <iostream>


Settings settings;


volatile bool quit;

void signalHandler(const int signum) {
    std::cout << std::endl << "Quitting..." << std::endl;

    quit = true;
    return;

    // Prevent warning
    int i = signum; i++;
}


bool poll_quit() {
    return quit;
}

void set_quit() {
    quit = true;
}

void reset_quit() {
    quit = false;
}
