# pingpong

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A ping-pong throughput test: the client sends blocks, the server echoes them back.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

Server:

```bash
cd build/output/bin
./pingpong_server [port [io_threads]]
```

Client (stress test):

```bash
./pingpong_client <ip> <port> <threads> <block_size> <sessions> <time_sec>
```

Example:

```bash
./pingpong_server 4567 4
./pingpong_client 127.0.0.1 4567 4 1024 2000 10
```

Logs:
- `pingpong_server_log.txt`
- `pingpong_client_log.txt`
