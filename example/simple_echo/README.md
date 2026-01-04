# simple_echo

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A minimal TCP echo server that sends back any bytes it receives.

## Build

From repo root:

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
cd build/output/bin
./simple_echo [port [io_threads]]
```

Defaults:
- port: 4567
- io_threads: 0 (single-threaded loop)

Log file: `simple_echo_log.txt` in the current working directory.
