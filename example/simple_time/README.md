# simple_time

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A minimal TCP time server that returns current time to clients.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
cd build/output/bin
./simple_time [port [io_threads]]
```

Defaults:
- port: 4567
- io_threads: 0

Log file: `simple_time_log.txt` in the current working directory.
