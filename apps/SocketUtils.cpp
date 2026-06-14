#include "SocketUtils.h"

#include <arpa/inet.h>
#include <cstdint>
#include <iostream>
#include <sys/socket.h> // recv, send

namespace SocketUtils {

    bool recvAll(int fd, char* buffer, std::size_t length) {
        std::size_t totalReceived = 0;

        while (totalReceived < length) {
            ssize_t n = recv(
                fd,
                buffer + totalReceived, // write into base + received so far
                length - totalReceived,
                0
            );

            if (n == -1) {
                return false;
            }

            if (n == 0) {
                return false;
            }

            totalReceived += static_cast<std::size_t>(n);
        }

        return true;
    }

    bool sendAll(int fd, const char* message, std::size_t length) {
        std::size_t totalSent = 0;

        while (totalSent < length) {
            ssize_t n = send(
                fd,
                message + totalSent, // start sending from base + current sent amount
                length - totalSent, // send whatever is left to send
                0
            );

            if (n == -1) {
                return false;
            }

            if (n == 0) {
                return false;
            }

            totalSent += static_cast<std::size_t>(n);
        }

        return true;
    }

    bool recvMessage(int fd, std::string& message) {
        std::uint32_t networkLength{};

        if (!SocketUtils::recvAll(
                fd,
                reinterpret_cast<char*>(&networkLength), // as recvall needs to write to a char*
                sizeof(networkLength)
            )) {
            std::cerr << "Receiving header failed\n";
            return false;
            }

        const std::uint32_t messageLength = ntohl(networkLength); // fix endian-ness

        if (messageLength > MAX_MESSAGE_SIZE) {
            std::cerr << "Message length is too large\n";
            return false;
        }

        message.resize(messageLength);

        if (messageLength > 0 &&
            !SocketUtils::recvAll(
                fd,
                message.data(),
                message.size()
            )) {
            std::cerr << "Receiving message failed\n";
            return false;
            }

        return true;
    }

    bool sendMessage(int fd, const std::string& message) {
        if (message.size() > MAX_MESSAGE_SIZE) {
            return false;
        }

        const auto messageSize =
            static_cast<std::uint32_t>(message.size());

        const std::uint32_t networkLength = htonl(messageSize);

        if (!SocketUtils::sendAll(
                fd,
                reinterpret_cast<const char*>(&networkLength),
                sizeof(networkLength)
            )) {
            return false;
            }

        if (!message.empty() &&
            !SocketUtils::sendAll(
                fd,
                message.data(),
                message.size()
            )) {
            return false;
            }

        return true;
    }

}
