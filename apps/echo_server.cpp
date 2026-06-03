#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

using namespace std;

int main() {
    addrinfo hints{};          // criteria for what kind of socket address we want
    addrinfo* res = nullptr;   // getaddrinfo will set this to the head of a result linked list

    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // suitable for bind(); nullptr means local interfaces

    // Populate res with local address info suitable for binding a server to port 8080.
    int status = getaddrinfo(nullptr, "8080", &hints, &res);

    if (status != 0) {
        cerr << "error occurred while getting address info: "
             << gai_strerror(status) << endl;
        return 1;
    }

    int finalSocketFd = -1;

    // Try each result and bind to the first one successfully.
    for (auto p = res; p != nullptr; p = p->ai_next) {
        // Create a socket. The returned int is a file descriptor:
        // an OS handle referring to this socket.
        int socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (socketFd == -1) {
            continue;
        }

        // Bind attaches the socket to a local IP address and port.
        if (bind(socketFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketFd);
            cerr << "bind error" << endl;
            continue;
        }

        finalSocketFd = socketFd;
        break;
    }

    freeaddrinfo(res);

    if (finalSocketFd == -1) {
        cerr << "Failed to bind" << endl;
        return 1;
    }

    if (listen(finalSocketFd, SOMAXCONN) == -1) {
        cerr << "listen failed" << endl;
        close(finalSocketFd);
        return 1;
    }

    cout << "Server is listening on port 8080" << endl;

    close(finalSocketFd);

    return 0;
}