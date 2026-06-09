// server.cpp

#include <iostream>   // std::cout, std::cerr
#include <array>      // std::array fixed-size buffer
#include <cstring>    // C string utilities, not heavily used here

#include <sys/types.h>   // system data types used by sockets
#include <sys/socket.h>  // socket(), bind(), listen(), accept(), recv(), send()
#include <netdb.h>       // getaddrinfo(), freeaddrinfo(), gai_strerror(), addrinfo
#include <unistd.h>      // close()

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
    // AI_PASSIVE means: give us an address suitable for bind().
    // Because this is a server, we want to bind to a local port.

    addrinfo* res = nullptr;
    // res will point to the head of a linked list of possible address results.

    /*
        Since node/hostname is nullptr and AI_PASSIVE is set,
        getaddrinfo gives us local addresses suitable for binding.

        "8080" is the service/port.
    */
    int status = getaddrinfo(nullptr, "8080", &hints, &res);

    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        return 1;
    }

    int serverFd = -1;
    // This will store the listening socket file descriptor.
    // A file descriptor is an integer handle used by the OS to refer to an open socket/file.

    /*
        getaddrinfo can return multiple possible addresses.
        Example: one IPv6 result and one IPv4 result.

        We try each result until socket() + bind() succeeds.
    */
    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        /*
            socket() creates a socket using this address result's:
            - family: IPv4/IPv6
            - type: TCP/UDP
            - protocol: usually TCP here

            It returns a file descriptor if successful.
        */
        serverFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (serverFd == -1) {
            // Failed to create socket for this address result.
            // Try the next result.
            continue;
        }

        /*
            SO_REUSEADDR allows us to reuse the port quickly after restarting the server.

            Without this, if you stop and restart your server quickly,
            bind() may fail because the port is temporarily still considered in use.
        */
        int yes = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            std::cerr << "setsockopt failed\n";
            close(serverFd);
            serverFd = -1;
            continue;
        }

        /*
            bind() attaches the socket to a local IP address and port.

            p->ai_addr contains the actual address.
            p->ai_addrlen tells bind() how large that address structure is.
        */
        if (bind(serverFd, p->ai_addr, p->ai_addrlen) == -1) {
            close(serverFd);
            serverFd = -1;
            continue;
        }

        // If we got here, socket() and bind() succeeded.
        break;
    }

    /*
        getaddrinfo allocated the linked list of results.
        Once we are done trying them, we free that memory.
    */
    freeaddrinfo(res);

    if (serverFd == -1) {
        std::cerr << "Failed to create and bind server socket\n";
        return 1;
    }

    /*
        listen() marks the socket as a listening socket.

        Before listen():
            socket is just bound to a port.

        After listen():
            socket can accept incoming TCP connection requests.

        SOMAXCONN is the maximum backlog value provided by the system.
    */
    if (listen(serverFd, SOMAXCONN) == -1) {
        std::cerr << "listen failed\n";
        close(serverFd);
        return 1;
    }

    std::cout << "Server listening on port 8080...\n";

    /*
        Main server loop.

        This server is blocking and single-threaded:
        - It accepts one client.
        - Handles that client until it disconnects.
        - Then goes back to accept another client.
    */
    while (true) {
        sockaddr_storage clientAddr{};
        // sockaddr_storage is large enough to store either IPv4 or IPv6 client address info.

        socklen_t clientAddrLen = sizeof(clientAddr);
        // accept() needs to know how much space is available for the client address.

        /*
            accept() waits for an incoming client connection.

            Important:
            serverFd is the listening socket.
            clientFd is a new connected socket for this specific client.

            We use clientFd for recv()/send().
            serverFd remains open to accept future clients.
        */
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

        std::array<char, 4096> buffer{};
        // Temporary buffer used to receive bytes from the client.

        /*
            Client handling loop.

            recv() blocks until:
            - data arrives
            - client disconnects
            - an error occurs
        */
        while (true) {
            ssize_t bytesReceived = recv(clientFd, buffer.data(), buffer.size(), 0);

            if (bytesReceived == -1) {
                // recv failed due to an error.
                std::cerr << "recv failed\n";
                break;
            }

            if (bytesReceived == 0) {
                /*
                    recv() returning 0 means the client closed the connection cleanly.
                */
                std::cout << "Client disconnected\n";
                break;
            }

            /*
                Echo server behavior:
                send back exactly the bytes we received.
            */
            ssize_t bytesSent = send(clientFd, buffer.data(), bytesReceived, 0);

            if (bytesSent == -1) {
                std::cerr << "send failed\n";
                break;
            }

            /*
                Small simplification:
                We assume send() sends all bytes.

                For Phase 1, this is okay.
                Later, we will handle partial sends properly.
            */
        }

        /*
            We are done with this client.
            Close only the connected client socket.
            The listening socket serverFd stays open.
        */
        close(clientFd);
    }

    /*
        In this infinite-loop server, this is normally not reached.
        But if the loop ever exits, close the listening socket.
    */
    close(serverFd);
    return 0;
}