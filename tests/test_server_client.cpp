#include "SocketUtils.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <csignal>

namespace {

constexpr const char* kTestPort = "18080";

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

int connectToServer()
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    const int status = getaddrinfo("localhost", kTestPort, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << '\n';
        return -1;
    }

    int socketFd = -1;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socketFd == -1) {
            continue;
        }

        if (connect(socketFd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(socketFd);
        socketFd = -1;
    }

    freeaddrinfo(res);
    return socketFd;
}

bool waitForServer()
{
    for (int attempt = 0; attempt < 50; ++attempt) {
        const int fd = connectToServer();
        if (fd != -1) {
            close(fd);
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

struct ServerProcess {
    pid_t pid = -1;

    explicit ServerProcess(const char* serverPath)
    {
        pid = fork();
        if (pid == -1) {
            std::cerr << "fork failed\n";
            std::exit(1);
        }

        if (pid == 0) {
            execl(serverPath, serverPath, kTestPort, static_cast<char*>(nullptr));
            _exit(127);
        }
    }

    ~ServerProcess()
    {
        if (pid > 0) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }

    ServerProcess(const ServerProcess&) = delete;
    ServerProcess& operator=(const ServerProcess&) = delete;
};

bool roundTrip(int fd, const std::string& message)
{
    std::string response;
    return SocketUtils::sendMessage(fd, message) &&
           SocketUtils::recvMessage(fd, response) &&
           response == message;
}

bool test_single_client_multiple_messages()
{
    const int fd = connectToServer();
    if (!expect(fd != -1, "single client should connect")) {
        return false;
    }

    const bool ok =
        expect(roundTrip(fd, "first framed message"), "server should echo first message") &&
        expect(roundTrip(fd, "second framed message"), "server should echo second message on same connection");

    close(fd);
    return ok;
}

bool test_multiple_clients()
{
    constexpr int kClientCount = 5;
    std::vector<std::thread> threads;
    std::vector<bool> results(kClientCount, false);

    for (int i = 0; i < kClientCount; ++i) {
        threads.emplace_back([i, &results] {
            const int fd = connectToServer();
            if (fd == -1) {
                results[i] = false;
                return;
            }

            const std::string first = "client " + std::to_string(i) + " first";
            const std::string second = "client " + std::to_string(i) + " second";

            results[i] = roundTrip(fd, first) && roundTrip(fd, second);
            close(fd);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    bool ok = true;
    for (bool result : results) {
        ok = result && ok;
    }

    return expect(ok, "all concurrent clients should receive their own echoed messages");
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "usage: server_client_tests <path-to-echo-server>\n";
        return 1;
    }

    ServerProcess server(argv[1]);

    if (!expect(waitForServer(), "server should start accepting connections")) {
        return 1;
    }

    bool ok = true;
    ok = test_single_client_multiple_messages() && ok;
    ok = test_multiple_clients() && ok;

    if (!ok) {
        return 1;
    }

    std::cout << "Server/client integration tests passed\n";
    return 0;
}
