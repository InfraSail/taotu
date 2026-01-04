# time_service

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A simple protobuf-based RPC Time Service with a sync client.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

Server:

```bash
cd build/output/bin
./time_service_server
```

Client:

```bash
./time_service_sync_client
```

Logs:
- `time_service_server_log.txt`
- `time_service_sync_client_log.txt`
