// server.cpp

#include <iostream>   // std::cout, std::cerr
#include <array>      // std::array fixed-size buffer
#include <cstring>    // C string utilities, not heavily used here
#include <thread>

#include <sys/types.h>   // system data types used by sockets
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), recv(), send()
#include <netdb.h>       // getaddrinfo(), freeaddrinfo(), gai_strerror(), addrinfo
#include <unistd.h>      // close()


void handleClient(int clientFd) {
        std::array<char, 4096> buffer{};
        
    while (true) {
        ssize_t bytesReceived = recv(clientFd, buffer.data(), buffer.size(), 0);

        if (bytesReceived == -1) {
            std::cerr << "recv failed\n";
            break;
        }

        if (bytesReceived == 0) {
            std::cout << "Client disconnected\n";
            break;
        }

        ssize_t bytesSent = send(clientFd, buffer.data(), bytesReceived, 0);

        if (bytesSent == -1) {
            std::cerr << "send failed\n";
            break;
        }
    }


    close(clientFd);

}

int main() {
    /*
        addrinfo is used with getaddrinfo().

        hints = tells getaddrinfo what kind of address/socket we want.
        res   = getaddrinfo will fill this with matching address results.
    */
    addrinfo hints{};

    hints.ai_family = AF_UNSPEC;
    // AF_UNSPEC means: allow either IPv4 or IPv6.

    hints.ai_socktype = SOCK_STREAM;
    // SOCK_STREAM means: TCP socket.
    // TCP gives us a reliable byte stream.

    hints.ai_flags = AI_PASSIVE;

    addrinfo* res = nullptr;

    int status = getaddrinfo(nullptr, "8080", &hints, &res);

    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        return 1;
    }

    int serverFd = -1;

    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        serverFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (serverFd == -1) {
            // Failed to create socket for this address result.
            // Try the next result.
            continue;
        }

        int yes = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            std::cerr << "setsockopt failed\n";
            close(serverFd);
            serverFd = -1;
            continue;
        }

        if (bind(serverFd, p->ai_addr, p->ai_addrlen) == -1) {
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
        std::cerr << "listen failed\n";
        close(serverFd);
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    while (true) {
        sockaddr_storage clientAddr{};

        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientFd = accept(
            serverFd,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrLen
        );

        if (clientFd == -1) {
            std::cerr << "accept failed\n";
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