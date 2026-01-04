# chat_room

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A simple chat room demo: server broadcasts each client message to all peers.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

Server:

```bash
cd build/output/bin
./chat_server [port [io_threads]]
```

Client:

```bash
./chat_client <ip> <port>
```

Type lines in the client terminal and they will be broadcast.

Logs:
- `server_main_log.txt` (server)
- `chat_client_log.txt` (client)
