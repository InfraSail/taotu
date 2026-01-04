# http_server

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个最小 HTTP 服务器示例，内置少量固定路由。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
cd build/output/bin
./http_server [端口 [IO线程数]]
```

测试：

```bash
curl http://127.0.0.1:4567/
curl http://127.0.0.1:4567/hello
```

日志文件：当前目录下的 `http_server_log.txt`。
