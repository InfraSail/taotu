# taotu

_[English](README.md) | [简体中文](README_zh-Hans.md)_

!["taotu" logo](./img/taotu.jpg)

A lightweight C++ network library based on the concurrent Reactor model, with io_uring support and a set of runnable demos.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Optional Release build:

```bash
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j
```

Notes:
- Requires a C++17 compiler, CMake, and liburing.
- RPC demo uses protobuf.
- You can tune io_uring entries with `TAOTU_IORING_ENTRIES` if memory is tight.

## Run demos

Binaries are placed under `build/output/bin` (or `build_release/output/bin`). Each demo has its own README under `example/`.

Example:

```bash
cd build/output/bin
./simple_echo 4567 4
```

## Basic usage

High-level flow:
1. Create an `EventManager` (or a pool of them).
2. Create `Server`/`Client` or demo-specific wrappers.
3. Register callbacks for connection/message/write/close events.
4. Start the event loop.

The `example/` directory contains minimal servers/clients showing common patterns.
