# muduo_modernCpp

A modern C++ implementation of the Muduo network library components, focusing on high-performance event handling and multi-threading support.

[中文文档](#中文文档)

## Features
- Modern C++17 implementation
- Event-driven networking
- High-performance event handling (epoll)
- Thread management and thread pool
- Non-blocking I/O operations

## Build Requirements
- CMake 3.10+
- C++17 compiler
- Linux platform (epoll support)

## Quick Start
```bash
# Build
mkdir build && cd build
cmake ..
make

# The library will be generated in lib/
```

## Core Components
- Event handling (Channel, EPollPoller)
- Thread management (Thread, EventLoopThread)
- Event loop (EventLoop, EventLoopThreadPool)
- Logging system

---

# 中文文档

muduo_modernCpp 是 muduo 网络库的现代 C++ 实现版本，专注于高性能事件处理和多线程支持。

## 特性
- 采用现代 C++17 实现
- 事件驱动的网络处理
- 高性能事件处理（epoll）
- 线程管理和线程池
- 非阻塞 I/O 操作

## 构建要求
- CMake 3.10+
- 支持 C++17 的编译器
- Linux 平台（支持 epoll）

## 快速开始
```bash
# 构建
mkdir build && cd build
cmake ..
make

# 生成的库文件在 lib/ 目录下
```

## 核心组件
- 事件处理（Channel、EPollPoller）
- 线程管理（Thread、EventLoopThread）
- 事件循环（EventLoop、EventLoopThreadPool）
- 日志系统