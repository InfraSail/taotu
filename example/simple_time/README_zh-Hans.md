# simple_time

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个最小的 TCP 时间服务器：向客户端返回当前时间。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
cd build/output/bin
./simple_time [端口 [IO线程数]]
```

默认值：
- 端口：4567
- IO线程数：0

日志文件：当前目录下的 `simple_time_log.txt`。
