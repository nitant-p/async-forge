# TCP Socket API Notes

This section documents the main structs and functions used in the Phase 1 blocking TCP echo server and client.

The code uses the POSIX/Berkeley sockets API, which is the standard low-level socket API available on Unix-like systems such as macOS and Linux.

---

## Core Mental Model

A TCP server and client both use sockets, but their setup flow is different.

```text
Server:
getaddrinfo() -> socket() -> setsockopt() -> bind() -> listen() -> accept() -> recv()/send() -> close()

Client:
getaddrinfo() -> socket() -> connect() -> send()/recv() -> close()
```

The server prepares a known address and waits for clients.

The client finds the server address and connects to it.

---

## File Descriptor

Most socket functions return or accept an `int` file descriptor.

```cpp
int socketFd;
```

A file descriptor is an integer handle given by the operating system.

It represents an open resource, such as:

```text
file
socket
pipe
terminal
```

For sockets, the file descriptor is used by functions such as:

```cpp
bind()
listen()
accept()
connect()
recv()
send()
close()
```

Example:

```cpp
int serverFd = socket(...);
```

Here, `serverFd` is not the socket object itself. It is an integer handle that refers to the socket inside the operating system.

---

# Structs

## `addrinfo`

Header:

```cpp
#include <netdb.h>
```

`addrinfo` is used with `getaddrinfo()`.

It describes socket address information.

In this project, we use two `addrinfo` variables:

```cpp
addrinfo hints{};
addrinfo* res = nullptr;
```

`hints` tells `getaddrinfo()` what kind of result we want.

`res` is filled by `getaddrinfo()` with a linked list of possible address results.

---

## `addrinfo hints`

Example:

```cpp
addrinfo hints{};
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;
```

Important fields:

| Field         | Meaning                                     |
| ------------- | ------------------------------------------- |
| `ai_family`   | Address family, such as IPv4 or IPv6        |
| `ai_socktype` | Socket type, such as TCP or UDP             |
| `ai_protocol` | Specific protocol, usually left as `0`      |
| `ai_flags`    | Extra options for `getaddrinfo()`           |
| `ai_addr`     | Pointer to actual socket address            |
| `ai_addrlen`  | Size of the address pointed to by `ai_addr` |
| `ai_next`     | Pointer to next result in linked list       |

---

## `ai_family`

Common values:

```cpp
AF_INET
AF_INET6
AF_UNSPEC
```

Meaning:

| Value       | Meaning                   |
| ----------- | ------------------------- |
| `AF_INET`   | IPv4 only                 |
| `AF_INET6`  | IPv6 only                 |
| `AF_UNSPEC` | Allow either IPv4 or IPv6 |

In this project:

```cpp
hints.ai_family = AF_UNSPEC;
```

This means:

```text
Give me either IPv4 or IPv6 results.
```

---

## `ai_socktype`

Common values:

```cpp
SOCK_STREAM
SOCK_DGRAM
```

Meaning:

| Value         | Meaning |
| ------------- | ------- |
| `SOCK_STREAM` | TCP     |
| `SOCK_DGRAM`  | UDP     |

In this project:

```cpp
hints.ai_socktype = SOCK_STREAM;
```

This means:

```text
We want TCP sockets.
```

TCP gives us a reliable byte stream.

---

## `ai_flags`

For the server:

```cpp
hints.ai_flags = AI_PASSIVE;
```

`AI_PASSIVE` means:

```text
Return addresses suitable for bind().
```

This is useful for servers because servers bind to a local port.

For the client, we usually do not set `AI_PASSIVE`, because the client is not binding to a known local port. The client is connecting to a remote server.

---

## `addrinfo* res`

Example:

```cpp
addrinfo* res = nullptr;
getaddrinfo(nullptr, "8080", &hints, &res);
```

`res` points to a linked list of possible address results.

That is why we loop through it:

```cpp
for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
    ...
}
```

`getaddrinfo()` may return multiple results, such as:

```text
IPv6 result
IPv4 result
```

We try each one until one works.

---

## `sockaddr`

Header:

```cpp
#include <sys/socket.h>
```

`sockaddr` is the generic socket address type used by C socket APIs.

Many functions expect a `sockaddr*`, for example:

```cpp
bind()
connect()
accept()
```

But in real code, the actual address may be IPv4 or IPv6.

So the actual data might be stored as:

```cpp
sockaddr_in    // IPv4
sockaddr_in6   // IPv6
```

The API uses `sockaddr*` as a generic pointer type.

---

## `sockaddr_storage`

Example:

```cpp
sockaddr_storage clientAddr{};
```

`sockaddr_storage` is a generic storage type large enough to hold either an IPv4 or IPv6 address.

We use it with `accept()`:

```cpp
sockaddr_storage clientAddr{};
socklen_t clientAddrLen = sizeof(clientAddr);

int clientFd = accept(
    serverFd,
    reinterpret_cast<sockaddr*>(&clientAddr),
    &clientAddrLen
);
```

The reason for the cast is that `accept()` expects:

```cpp
sockaddr*
```

but we are passing:

```cpp
sockaddr_storage*
```

So we cast it to the generic socket address pointer type.

---

## `socklen_t`

Example:

```cpp
socklen_t clientAddrLen = sizeof(clientAddr);
```

`socklen_t` is an integer type used for socket address lengths.

Functions such as `accept()`, `bind()`, and `connect()` need to know the size of the address struct they are using.

---

# Functions

## `getaddrinfo()`

Header:

```cpp
#include <netdb.h>
```

Signature:

```cpp
int getaddrinfo(
    const char* node,
    const char* service,
    const addrinfo* hints,
    addrinfo** res
);
```

Purpose:

```text
Convert a hostname/service into socket address structures.
```

It does not create a socket.

It only gives you address information that can later be used with:

```cpp
socket()
bind()
connect()
```

---

## Parameters

### `node`

The hostname or IP address.

Server example:

```cpp
getaddrinfo(nullptr, "8080", &hints, &res);
```

For the server, `node` is `nullptr` because we want local addresses suitable for binding.

Client example:

```cpp
getaddrinfo("localhost", "8080", &hints, &res);
```

For the client, `node` is `"localhost"` because we want to connect to a server running on localhost.

---

### `service`

The port or service name.

Example:

```cpp
"8080"
```

This means port 8080.

---

### `hints`

A pointer to an `addrinfo` struct describing what kind of results we want.

Example:

```cpp
&hints
```

---

### `res`

A pointer to an `addrinfo*`.

Example:

```cpp
&res
```

This is a double pointer because `getaddrinfo()` needs to modify `res` so that it points to the result linked list.

---

## Return value

`getaddrinfo()` returns:

```text
0 on success
non-zero error code on failure
```

Example:

```cpp
int status = getaddrinfo(...);

if (status != 0) {
    std::cerr << gai_strerror(status) << "\n";
}
```

Use `gai_strerror()` to convert the error code into a readable message.

---

## `freeaddrinfo()`

Header:

```cpp
#include <netdb.h>
```

Signature:

```cpp
void freeaddrinfo(addrinfo* res);
```

Purpose:

```text
Free the linked list allocated by getaddrinfo().
```

Example:

```cpp
freeaddrinfo(res);
```

Call this once you are done using the address results.

---

## `socket()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int socket(int domain, int type, int protocol);
```

Purpose:

```text
Create a new socket and return a file descriptor.
```

Example:

```cpp
int socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
```

---

## Parameters

### `domain`

Address family.

Examples:

```cpp
AF_INET
AF_INET6
AF_UNSPEC
```

In our code, we use:

```cpp
p->ai_family
```

because `getaddrinfo()` already selected the address family for that result.

---

### `type`

Socket type.

Examples:

```cpp
SOCK_STREAM
SOCK_DGRAM
```

In our code:

```cpp
p->ai_socktype
```

For TCP, this is `SOCK_STREAM`.

---

### `protocol`

Specific protocol.

Usually:

```cpp
0
```

or the value from `getaddrinfo()`:

```cpp
p->ai_protocol
```

---

## Return value

Returns:

```text
socket file descriptor on success
-1 on failure
```

Example:

```cpp
if (socketFd == -1) {
    continue;
}
```

---

## `setsockopt()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int setsockopt(
    int socket,
    int level,
    int option_name,
    const void* option_value,
    socklen_t option_len
);
```

Purpose:

```text
Set an option on a socket.
```

In the server, we use:

```cpp
int yes = 1;

setsockopt(
    serverFd,
    SOL_SOCKET,
    SO_REUSEADDR,
    &yes,
    sizeof(yes)
);
```

This allows the server to reuse the port quickly after restarting.

---

## Parameters

### `socket`

The socket file descriptor.

Example:

```cpp
serverFd
```

---

### `level`

The level at which the option is defined.

Example:

```cpp
SOL_SOCKET
```

This means the option is a general socket-level option.

---

### `option_name`

The specific option to set.

Example:

```cpp
SO_REUSEADDR
```

This allows address reuse.

---

### `option_value`

Pointer to the value for the option.

Example:

```cpp
&yes
```

---

### `option_len`

Size of the option value.

Example:

```cpp
sizeof(yes)
```

---

## Return value

Returns:

```text
0 on success
-1 on failure
```

---

## `bind()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int bind(
    int socket,
    const sockaddr* address,
    socklen_t address_len
);
```

Purpose:

```text
Attach a socket to a local IP address and port.
```

Only the server uses `bind()` in this project.

Example:

```cpp
bind(serverFd, p->ai_addr, p->ai_addrlen);
```

---

## Parameters

### `socket`

The socket file descriptor.

Example:

```cpp
serverFd
```

---

### `address`

The local address to bind to.

Example:

```cpp
p->ai_addr
```

This comes from `getaddrinfo()`.

---

### `address_len`

The size of the address structure.

Example:

```cpp
p->ai_addrlen
```

---

## Return value

Returns:

```text
0 on success
-1 on failure
```

---

## `listen()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int listen(int socket, int backlog);
```

Purpose:

```text
Mark a bound socket as a listening socket.
```

Before `listen()`, the socket is only bound to a local address.

After `listen()`, the socket can accept incoming TCP connection requests.

Example:

```cpp
listen(serverFd, SOMAXCONN);
```

---

## Parameters

### `socket`

The socket file descriptor.

Example:

```cpp
serverFd
```

This must be a socket that was successfully created and bound.

---

### `backlog`

The maximum length of the pending connection queue.

Example:

```cpp
SOMAXCONN
```

When clients connect, the OS keeps completed connection requests in a queue until the server calls `accept()`.

`backlog` controls the approximate maximum size of that queue.

---

## Return value

Returns:

```text
0 on success
-1 on failure
```

If `listen()` fails in this Phase 1 project, we close the socket and exit.

---

## `accept()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int accept(
    int socket,
    sockaddr* address,
    socklen_t* address_len
);
```

Purpose:

```text
Accept one incoming client connection from the listening socket.
```

Example:

```cpp
int clientFd = accept(
    serverFd,
    reinterpret_cast<sockaddr*>(&clientAddr),
    &clientAddrLen
);
```

---

## Parameters

### `socket`

The listening socket.

Example:

```cpp
serverFd
```

---

### `address`

Pointer to storage where the client address will be written.

Example:

```cpp
reinterpret_cast<sockaddr*>(&clientAddr)
```

You can pass `nullptr` if you do not care about the client address.

---

### `address_len`

Pointer to the size of the address storage.

Example:

```cpp
&clientAddrLen
```

`accept()` may update this with the actual size of the client address.

---

## Return value

Returns:

```text
new connected client socket descriptor on success
-1 on failure
```

Important distinction:

```text
serverFd = listening socket
clientFd = connected socket for one client
```

Use `serverFd` only for accepting new clients.

Use `clientFd` for `recv()` and `send()`.

---

## `connect()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
int connect(
    int socket,
    const sockaddr* address,
    socklen_t address_len
);
```

Purpose:

```text
Connect a client socket to a server address.
```

Only the client uses `connect()` in this project.

Example:

```cpp
connect(socketFd, p->ai_addr, p->ai_addrlen);
```

---

## Parameters

### `socket`

The client socket file descriptor.

Example:

```cpp
socketFd
```

---

### `address`

The server address to connect to.

Example:

```cpp
p->ai_addr
```

This comes from `getaddrinfo("localhost", "8080", ...)`.

---

### `address_len`

The size of the server address structure.

Example:

```cpp
p->ai_addrlen
```

---

## Return value

Returns:

```text
0 on success
-1 on failure
```

After `connect()` succeeds, the client can use:

```cpp
send()
recv()
```

---

## `recv()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
ssize_t recv(
    int socket,
    void* buffer,
    size_t length,
    int flags
);
```

Purpose:

```text
Receive bytes from a connected socket.
```

Example:

```cpp
ssize_t bytesReceived = recv(
    clientFd,
    buffer.data(),
    buffer.size(),
    0
);
```

---

## Parameters

### `socket`

A connected socket.

On the server:

```cpp
clientFd
```

On the client:

```cpp
socketFd
```

---

### `buffer`

Pointer to memory where received bytes should be written.

Example:

```cpp
buffer.data()
```

---

### `length`

Maximum number of bytes to receive.

Example:

```cpp
buffer.size()
```

---

### `flags`

Usually:

```cpp
0
```

This means normal receive behavior.

---

## Return value

Returns:

```text
positive number = number of bytes received
0               = peer closed connection cleanly
-1              = error
```

Example:

```cpp
if (bytesReceived == 0) {
    std::cout << "Client disconnected\n";
}
```

---

## `send()`

Header:

```cpp
#include <sys/socket.h>
```

Signature:

```cpp
ssize_t send(
    int socket,
    const void* buffer,
    size_t length,
    int flags
);
```

Purpose:

```text
Send bytes through a connected socket.
```

Example:

```cpp
ssize_t bytesSent = send(
    clientFd,
    buffer.data(),
    bytesReceived,
    0
);
```

---

## Parameters

### `socket`

A connected socket.

On the server:

```cpp
clientFd
```

On the client:

```cpp
socketFd
```

---

### `buffer`

Pointer to the bytes to send.

Example:

```cpp
buffer.data()
```

---

### `length`

Number of bytes to send.

Example:

```cpp
bytesReceived
```

or:

```cpp
message.size()
```

---

### `flags`

Usually:

```cpp
0
```

This means normal send behavior.

---

## Return value

Returns:

```text
number of bytes sent on success
-1 on failure
```

Important Phase 1 simplification:

```text
We assume send() sends all requested bytes.
```

In real systems, `send()` can send fewer bytes than requested. Later phases should handle partial sends.

---

## `close()`

Header:

```cpp
#include <unistd.h>
```

Signature:

```cpp
int close(int fd);
```

Purpose:

```text
Close an open file descriptor.
```

For sockets, this releases the socket resource.

Examples:

```cpp
close(clientFd);
close(serverFd);
close(socketFd);
```

---

## Return value

Returns:

```text
0 on success
-1 on failure
```

In this project, we usually call `close()` during cleanup and do not do complex error handling around it.

---

# C++ Cast Used

## `reinterpret_cast`

Example:

```cpp
reinterpret_cast<sockaddr*>(&clientAddr)
```

Purpose:

```text
Treat a pointer as a different pointer type.
```

In socket code, this is commonly used because C socket APIs expect generic `sockaddr*` pointers.

We store the client address in:

```cpp
sockaddr_storage clientAddr{};
```

But `accept()` expects:

```cpp
sockaddr*
```

So we cast:

```cpp
reinterpret_cast<sockaddr*>(&clientAddr)
```

This is safe in this context because `sockaddr_storage` is specifically designed to store socket address structures.

---

# Important Server vs Client Difference

## Server

The server has two kinds of socket descriptors:

```text
serverFd = listening socket
clientFd = connected socket returned by accept()
```

The server uses:

```cpp
bind()
listen()
accept()
```

The server communicates using the socket returned by `accept()`:

```cpp
recv(clientFd, ...)
send(clientFd, ...)
```

---

## Client

The client has one main socket descriptor:

```text
socketFd = connected socket to the server
```

The client uses:

```cpp
connect()
```

After connecting, it communicates using:

```cpp
send(socketFd, ...)
recv(socketFd, ...)
```

---

# Summary Table

| Function        | Server |     Client | Purpose                        |
| --------------- | -----: | ---------: | ------------------------------ |
| `getaddrinfo()` |    Yes |        Yes | Get address structures         |
| `socket()`      |    Yes |        Yes | Create socket                  |
| `setsockopt()`  |    Yes |   Optional | Configure socket options       |
| `bind()`        |    Yes | Usually no | Attach socket to local IP/port |
| `listen()`      |    Yes |         No | Mark socket as listening       |
| `accept()`      |    Yes |         No | Accept incoming connection     |
| `connect()`     |     No |        Yes | Connect to server              |
| `recv()`        |    Yes |        Yes | Receive bytes                  |
| `send()`        |    Yes |        Yes | Send bytes                     |
| `close()`       |    Yes |        Yes | Close socket/file descriptor   |

The most important distinction:

```text
Server prepares a known address and waits.
Client connects to that known address.
```
