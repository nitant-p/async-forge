// client.cpp
#include <iostream>
#include <string>
#include <array>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

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

    std::string message;
    std::cout << "Enter message: ";
    std::getline(std::cin, message);

    ssize_t bytesSent = send(socketFd, message.data(), message.size(), 0);
    if (bytesSent == -1) {
        std::cerr << "send failed\n";
        close(socketFd);
        return 1;
    }

    std::array<char, 4096> buffer{};
    ssize_t bytesReceived = recv(socketFd, buffer.data(), buffer.size(), 0);

    if (bytesReceived == -1) {
        std::cerr << "recv failed\n";
        close(socketFd);
        return 1;
    }

    std::string response(buffer.data(), bytesReceived);
    std::cout << "Server echoed: " << response << "\n";

    close(socketFd);
    return 0;
}