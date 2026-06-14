// server.cpp

#include <iostream>   // std::cout, std::cerr
#include <string>
#include <thread>
#include <cerrno>
#include <cstring>

#include <sys/types.h>   // system data types used by sockets
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), recv(), send()
#include <netdb.h>       // getaddrinfo(), freeaddrinfo(), gai_strerror(), addrinfo
#include <unistd.h>      // close()

#include "SocketUtils.h"

// attempt to receive all bytes and put into buffer
// repeat until specified length has been received


void handleClient(int clientFd) {
    while (true) {
        std::string messageBuffer;
        if (!SocketUtils::recvMessage(clientFd, messageBuffer)) {
            std::cerr << "Error recieving message.\n";
            break;
        }

        // send back
        if (!SocketUtils::sendMessage(clientFd, messageBuffer)) {
            std::cerr << "Error sending message.\n";
            break;
        }

        
    }


    close(clientFd);

}

int main(int argc, char* argv[]) {
    /*
        addrinfo is used with getaddrinfo().

        hints = tells getaddrinfo what kind of address/socket we want.
        res   = getaddrinfo will fill this with matching address results.
    */
    addrinfo hints{};

    hints.ai_family = AF_INET;
    // AF_INET means: use IPv4 for this phase.

    hints.ai_socktype = SOCK_STREAM;
    // SOCK_STREAM means: TCP socket.
    // TCP gives us a reliable byte stream.

    hints.ai_flags = AI_PASSIVE;

    addrinfo* res = nullptr;

    const char* port = argc > 1 ? argv[1] : "8080";

    int status = getaddrinfo(nullptr, port, &hints, &res);

    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        return 1;
    }

    int serverFd = -1;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        serverFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (serverFd == -1) {
            std::cerr << "socket failed: " << std::strerror(errno) << "\n";
            // Failed to create socket for this address result.
            // Try the next result.
            continue;
        }

        int yes = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            std::cerr << "setsockopt failed: " << std::strerror(errno) << "\n";
            close(serverFd);
            serverFd = -1;
            continue;
        }

        if (bind(serverFd, p->ai_addr, p->ai_addrlen) == -1) {
            std::cerr << "bind failed: " << std::strerror(errno) << "\n";
            close(serverFd);
            serverFd = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (serverFd == -1) {
        std::cerr << "Failed to create and bind server socket\n";
        return 1;
    }

    if (listen(serverFd, SOMAXCONN) == -1) {
        std::cerr << "listen failed: " << std::strerror(errno) << "\n";
        close(serverFd);
        return 1;
    }

    std::cout << "Server listening on port " << port << "...\n";

    while (true) {
        sockaddr_storage clientAddr{};

        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientFd = accept(
            serverFd,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrLen
        );

        if (clientFd == -1) {
            std::cerr << "accept failed: " << std::strerror(errno) << "\n";
            continue;
        }

        std::cout << "Client connected\n";
        std::cout << "Hand off to worker thread.\n";

        std::thread t(handleClient, clientFd);

        // dont need to wait for thread to finish, continue making another connections
        t.detach();
    }

    close(serverFd);
    return 0;
}
