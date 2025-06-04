//crear un socket UDP
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

int sock = socket(AF_INET, SOCK_DGRAM, 0);
if (sock < 0) {
    std::cerr << "Error creating socket" << std::endl;
    return 1;
}

