#pragma once

#include <cstddef>   // size_t
#include <string>

inline constexpr std::size_t HEADER_SIZE = 4;
inline constexpr std::size_t MAX_MESSAGE_SIZE = 65'536;

namespace SocketUtils {

    bool recvAll(int fd, char* buffer, std::size_t length);

    bool sendAll(int fd, const char* message, std::size_t length);

    bool recvMessage(int fd, std::string& message);

    bool sendMessage(int fd, const std::string& message);

}
