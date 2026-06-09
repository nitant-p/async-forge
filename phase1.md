# Phase 1: Blocking TCP Echo Server and Client

This phase implements a simple blocking TCP echo system in C++.

The project contains two programs:

```text
server.cpp
client.cpp
```

The server listens on port `8080`, accepts a client connection, receives bytes from the client, and sends the same bytes back.

The client connects to the server, sends a message, receives the echoed response, prints it, and exits.

---

## Goal of Phase 1

The goal is to understand the basic TCP socket flow:

```text
Server:
socket() -> bind() -> listen() -> accept() -> recv() -> send() -> close()

Client:
socket() -> connect() -> send() -> recv() -> close()
```

This phase is intentionally simple:

* No threads
* No async I/O
* No message framing
* No multiple clients at the same time
* No partial-send handling yet

---

## Server vs Client: Main Difference

The server waits for incoming connections.

The client initiates a connection to the server.

```text
Server = waits and accepts
Client = connects and sends
```

---

## Server Flow

The server does the following:

```text
1. Resolve local bind address using getaddrinfo()
2. Create a socket using socket()
3. Bind the socket to port 8080 using bind()
4. Mark the socket as a listening socket using listen()
5. Wait for a client using accept()
6. Receive data from the client using recv()
7. Send the same data back using send()
8. Close the client socket
9. Continue waiting for another client
```

The important server-side APIs are:

```cpp
getaddrinfo()
socket()
setsockopt()
bind()
listen()
accept()
recv()
send()
close()
freeaddrinfo()
```

---

## Client Flow

The client does the following:

```text
1. Resolve the server address using getaddrinfo()
2. Create a socket using socket()
3. Connect to the server using connect()
4. Send a message using send()
5. Receive the echoed response using recv()
6. Print the response
7. Close the socket
```

The important client-side APIs are:

```cpp
getaddrinfo()
socket()
connect()
send()
recv()
close()
freeaddrinfo()
```

---

## Key API Difference

The server uses:

```cpp
bind()
listen()
accept()
```

The client does not.

The client uses:

```cpp
connect()
```

The server does not use `connect()` because it is not initiating the connection. It waits for clients to connect.

---

## Listening Socket vs Connected Socket

On the server, there are two different socket file descriptors:

```text
serverFd = listening socket
clientFd = connected socket for one client
```

`serverFd` is created by:

```cpp
socket()
bind()
listen()
```

It is used only to accept new connections:

```cpp
accept(serverFd, ...)
```

`clientFd` is returned by `accept()`:

```cpp
int clientFd = accept(serverFd, ...);
```

It is used to communicate with one specific client:

```cpp
recv(clientFd, ...)
send(clientFd, ...)
```

The server should not call `recv()` or `send()` on `serverFd`.

---

## Client Socket

The client only has one main socket:

```text
socketFd = connected socket to the server
```

It is created by:

```cpp
socket()
```

Then connected using:

```cpp
connect()
```

After that, the client can use:

```cpp
send(socketFd, ...)
recv(socketFd, ...)
```

---

## Why the Server Uses bind()

The server must use `bind()` because it needs to attach itself to a known local port.

For example:

```text
localhost:8080
```

Without `bind()`, clients would not know where to connect.

---

## Why the Client Does Not Usually Use bind()

The client usually does not care which local port it uses.

When the client calls `connect()`, the operating system automatically assigns it an available temporary local port.

So the client only needs to know the server's address and port.

---

## Why the Server Uses listen()

After binding to a port, the server calls:

```cpp
listen(serverFd, SOMAXCONN);
```

This tells the operating system:

```text
This socket is now ready to accept incoming TCP connection requests.
```

Before `listen()`, the socket is only bound to a port.

After `listen()`, it becomes a listening socket.

---

## Why the Server Uses accept()

The server calls:

```cpp
int clientFd = accept(serverFd, ...);
```

`accept()` blocks until a client connects.

When a client connects, `accept()` returns a new socket:

```text
clientFd
```

This new socket represents the connection between the server and that specific client.

---

## Why the Client Uses connect()

The client calls:

```cpp
connect(socketFd, ...)
```

This starts the TCP connection to the server.

After `connect()` succeeds, the client can send and receive data.

---

## Blocking Behavior

In Phase 1, all socket operations are blocking.

This means:

```cpp
accept()
```

waits until a client connects.

```cpp
recv()
```

waits until data arrives, the peer disconnects, or an error occurs.

Because the server is single-threaded, it can only handle one client at a time.

---

## Phase 1 Limitation

The server handles clients like this:

```text
Accept client A
Handle client A until it disconnects
Then accept client B
```

So if client A stays connected and does nothing, client B may have to wait.

This is fixed in Phase 2 by using one thread per client.

---

## How to Compile

On macOS or Linux:

```bash
clang++ -std=c++20 server.cpp -o server
clang++ -std=c++20 client.cpp -o client
```

Or using `g++`:

```bash
g++ -std=c++20 server.cpp -o server
g++ -std=c++20 client.cpp -o client
```

---

## How to Run

Start the server in one terminal:

```bash
./server
```

Then start the client in another terminal:

```bash
./client
```

Example client output:

```text
Connected to server
Enter message: hello
Server echoed: hello
```

Example server output:

```text
Server listening on port 8080...
Client connected
Client disconnected
```

---

## Summary

| Concept                       | Server                     | Client                                   |
| ----------------------------- | -------------------------- | ---------------------------------------- |
| Starts first?                 | Usually yes                | Usually no                               |
| Creates socket?               | Yes                        | Yes                                      |
| Uses `bind()`?                | Yes                        | Usually no                               |
| Uses `listen()`?              | Yes                        | No                                       |
| Uses `accept()`?              | Yes                        | No                                       |
| Uses `connect()`?             | No                         | Yes                                      |
| Uses `send()`?                | Yes                        | Yes                                      |
| Uses `recv()`?                | Yes                        | Yes                                      |
| Main role                     | Wait for clients           | Connect to server                        |
| Socket used for communication | `clientFd` from `accept()` | `socketFd` from `socket()` + `connect()` |

The most important distinction:

```text
Server prepares a known address and waits.
Client finds that address and connects.
```
