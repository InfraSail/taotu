# simple_echo

_[English](README.md) | [简体中文](README_zh-Hans.md)_

一个最小的 TCP 回显服务器：收到什么就回什么。

## 构建

在仓库根目录执行：

```bash
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
cd build/output/bin
./simple_echo [端口 [IO线程数]]
```

默认值：
- 端口：4567
- IO线程数：0（单线程）

日志文件：当前目录下的 `simple_echo_log.txt`。
