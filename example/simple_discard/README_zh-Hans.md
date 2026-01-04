# simple_discard

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个最小的 TCP 服务器：接收数据后直接丢弃。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
cd build/output/bin
./simple_discard [端口 [IO线程数]]
```

默认值：
- 端口：4567
- IO线程数：0

日志文件：当前目录下的 `simple_discard_log.txt`。
