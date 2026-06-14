// client.cpp
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "SocketUtils.h"

int main() {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP

    addrinfo* res = nullptr;

    int status = getaddrinfo("localhost", "8080", &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        return 1;
    }

    int socketFd = -1;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socketFd == -1) {
            continue;
        }

        if (connect(socketFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketFd);
            socketFd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (socketFd == -1) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    std::cout << "Connected to server\n";

    while (true) {
        std::string message;
        while (true) {

            std::cout << "Enter message: ";
            std::getline(std::cin, message);

            if (message.length() > MAX_MESSAGE_SIZE) {
                std::cerr << "message is too large. please re-enter\n";
            } else break;
        }

        if (!SocketUtils::sendMessage(socketFd, message)) {
            std::cerr << "Error sending message\n";
            break;
        }


        // get response now
        std::string response; // write into here
        if (!SocketUtils::recvMessage(socketFd, response)) {
            std::cerr << "Error recieving response\n";
            break;
        }

        std::cout << "Server echoed: " << response << "\n";
    }

    close(socketFd);
    return 0;
}
