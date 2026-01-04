# chat_room

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个简单聊天室示例：服务端会把某个客户端发来的消息广播给所有客户端。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

服务端：

```bash
cd build/output/bin
./chat_server [端口 [IO线程数]]
```

客户端：

```bash
./chat_client <ip> <端口>
```

在客户端终端输入一行文本即可广播。

日志：
- `server_main_log.txt`（服务端）
- `chat_client_log.txt`（客户端）
