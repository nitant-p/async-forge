# async-forge

`async-forge` is a learning project for building network servers in C++ step by step.

The project starts with raw blocking TCP sockets, then grows through thread-per-client concurrency, length-prefixed message framing, a small Redis-like key-value server, a hand-written event loop, Boost.Asio callbacks, and finally C++20 coroutine-based async networking.

The goal is to understand how server architecture changes as the same basic application moves from simple blocking I/O to reusable asynchronous infrastructure.

## Current Phase

Phase 0: project setup.

This phase establishes the build system, folder layout, basic executable targets, and test target.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Test

```bash
ctest --test-dir build
```

## Run

```bash
./build/echo_server
./build/echo_client
```
