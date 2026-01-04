# taotu

_[English](README.md) | [简体中文](README_zh-Hans.md)_

!["taotu" logo](./img/taotu.jpg)

一个基于并发 Reactor 模型的轻量级 C++ 网络库，支持 io_uring，并附带可直接运行的示例。

## 构建

```bash
cmake -S . -B build
cmake --build build -j
```

可选的 Release 构建：

```bash
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release -j
```

说明：
- 需要 C++17 编译器、CMake 和 liburing。
- RPC 示例需要 protobuf。
- 如果内存吃紧，可以通过 `TAOTU_IORING_ENTRIES` 调小 io_uring 队列大小。

## 运行示例

可执行文件位于 `build/output/bin`（或 `build_release/output/bin`）。每个示例在 `example/` 目录下都有独立 README。

示例：

```bash
cd build/output/bin
./simple_echo 4567 4
```

## 基本使用

一般流程：
1. 创建 `EventManager`（或线程池）。
2. 创建 `Server`/`Client` 或示例封装。
3. 注册连接/消息/写完成/关闭回调。
4. 启动事件循环。

`example/` 目录提供了最小可运行的模式参考。
