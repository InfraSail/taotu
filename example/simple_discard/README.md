# simple_discard

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A minimal TCP server that discards all incoming data.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
cd build/output/bin
./simple_discard [port [io_threads]]
```

Defaults:
- port: 4567
- io_threads: 0

Log file: `simple_discard_log.txt` in the current working directory.
