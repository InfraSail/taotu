# time_service

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个基于 protobuf 的 RPC 时间服务与同步客户端示例。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

服务端：

```bash
cd build/output/bin
./time_service_server
```

客户端：

```bash
./time_service_sync_client
```

日志：
- `time_service_server_log.txt`
- `time_service_sync_client_log.txt`
