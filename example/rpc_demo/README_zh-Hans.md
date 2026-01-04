# rpc_demo

_[English](README.md) | [简体中文](README_zh-Hans.md)_

RPC 示例集合，目前包含 Time Service。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 示例

- `time_service/`：基于 protobuf 的 RPC 服务与同步客户端。

具体用法见各示例目录的 README。
