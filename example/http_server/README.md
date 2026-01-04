# http_server

_[English](README.md) | [简体中文](README_zh-Hans.md)_

A minimal HTTP server demo with a few fixed routes.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
cd build/output/bin
./http_server [port [io_threads]]
```

Try:

```bash
curl http://127.0.0.1:4567/
curl http://127.0.0.1:4567/hello
```

Log file: `http_server_log.txt` in the current working directory.
