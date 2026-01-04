# rpc_demo

_[English](README.md) | [简体中文](README_zh-Hans.md)_

RPC demo collection. Currently includes the Time Service demo.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Demos

- `time_service/`: a simple protobuf-based RPC service and a sync client.

See each demo directory for usage.
