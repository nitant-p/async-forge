#include "SocketUtils.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

struct SocketPair {
    int fds[2]{-1, -1};

    SocketPair()
    {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
            std::cerr << "socketpair failed\n";
            std::exit(1);
        }
    }

    ~SocketPair()
    {
        if (fds[0] != -1) {
            close(fds[0]);
        }
        if (fds[1] != -1) {
            close(fds[1]);
        }
    }

    SocketPair(const SocketPair&) = delete;
    SocketPair& operator=(const SocketPair&) = delete;
};

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

bool test_round_trip_message()
{
    SocketPair sockets;

    const std::string sent = "hello framed world";
    std::string received;

    return expect(SocketUtils::sendMessage(sockets.fds[0], sent), "sendMessage should send a normal message") &&
           expect(SocketUtils::recvMessage(sockets.fds[1], received), "recvMessage should receive a normal message") &&
           expect(received == sent, "received message should match sent message");
}

bool test_empty_message()
{
    SocketPair sockets;

    const std::string sent;
    std::string received = "not empty";

    return expect(SocketUtils::sendMessage(sockets.fds[0], sent), "sendMessage should allow an empty message") &&
           expect(SocketUtils::recvMessage(sockets.fds[1], received), "recvMessage should receive an empty message") &&
           expect(received.empty(), "received empty message should be empty");
}

bool test_multiple_messages_back_to_back()
{
    SocketPair sockets;

    std::string first;
    std::string second;

    return expect(SocketUtils::sendMessage(sockets.fds[0], "one"), "first sendMessage should succeed") &&
           expect(SocketUtils::sendMessage(sockets.fds[0], "two"), "second sendMessage should succeed") &&
           expect(SocketUtils::recvMessage(sockets.fds[1], first), "first recvMessage should succeed") &&
           expect(SocketUtils::recvMessage(sockets.fds[1], second), "second recvMessage should succeed") &&
           expect(first == "one", "first message should be reconstructed correctly") &&
           expect(second == "two", "second message should be reconstructed correctly");
}

bool test_header_uses_network_byte_order()
{
    SocketPair sockets;

    const std::string sent = "abcde";
    char header[HEADER_SIZE]{};
    std::uint32_t networkLength = 0;

    return expect(SocketUtils::sendMessage(sockets.fds[0], sent), "sendMessage should send header and body") &&
           expect(SocketUtils::recvAll(sockets.fds[1], header, sizeof(header)), "recvAll should read the 4-byte header") &&
           (std::memcpy(&networkLength, header, sizeof(networkLength)), true) &&
           expect(ntohl(networkLength) == sent.size(), "header should encode body size in network byte order");
}

bool test_oversized_send_rejected()
{
    SocketPair sockets;

    const std::string oversized(MAX_MESSAGE_SIZE + 1, 'x');

    return expect(!SocketUtils::sendMessage(sockets.fds[0], oversized), "sendMessage should reject oversized messages");
}

bool test_oversized_receive_rejected()
{
    SocketPair sockets;

    const std::uint32_t networkLength = htonl(static_cast<std::uint32_t>(MAX_MESSAGE_SIZE + 1));
    std::string received;

    return expect(SocketUtils::sendAll(
                      sockets.fds[0],
                      reinterpret_cast<const char*>(&networkLength),
                      sizeof(networkLength)),
                  "sendAll should send an oversized header") &&
           expect(!SocketUtils::recvMessage(sockets.fds[1], received), "recvMessage should reject oversized length headers");
}

bool test_disconnect_mid_body_fails()
{
    SocketPair sockets;

    const std::uint32_t networkLength = htonl(5);
    const char partialBody[] = {'h', 'i'};
    std::string received;

    if (!expect(SocketUtils::sendAll(
                    sockets.fds[0],
                    reinterpret_cast<const char*>(&networkLength),
                    sizeof(networkLength)),
                "sendAll should send a valid header")) {
        return false;
    }

    if (!expect(SocketUtils::sendAll(sockets.fds[0], partialBody, sizeof(partialBody)),
                "sendAll should send a partial body")) {
        return false;
    }

    shutdown(sockets.fds[0], SHUT_WR);

    return expect(!SocketUtils::recvMessage(sockets.fds[1], received), "recvMessage should fail when peer disconnects mid-body");
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_round_trip_message() && ok;
    ok = test_empty_message() && ok;
    ok = test_multiple_messages_back_to_back() && ok;
    ok = test_header_uses_network_byte_order() && ok;
    ok = test_oversized_send_rejected() && ok;
    ok = test_oversized_receive_rejected() && ok;
    ok = test_disconnect_mid_body_fails() && ok;

    if (!ok) {
        return 1;
    }

    std::cout << "SocketUtils framing tests passed\n";
    return 0;
}
