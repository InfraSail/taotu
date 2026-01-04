# pingpong

_[English](README.md) | [简体中文](README_zh-Hans.md)_

Ping-Pong 吞吐测试：客户端发送数据块，服务端原样回传。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

服务端：

```bash
cd build/output/bin
./pingpong_server [端口 [IO线程数]]
```

客户端（压测）：

```bash
./pingpong_client <ip> <端口> <线程数> <块大小> <会话数> <时间秒>
```

示例：

```bash
./pingpong_server 4567 4
./pingpong_client 127.0.0.1 4567 4 1024 2000 10
```

日志：
- `pingpong_server_log.txt`
- `pingpong_client_log.txt`
