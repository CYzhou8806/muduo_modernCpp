cmake_minimum_required(VERSION 3.10)
project(muduo_modernCpp)

# Build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# Source files
set(SOURCES
        Channel.cpp
        EPollPoller.cpp
        EventLoop.cpp
        EventLoopThread.cpp
        EventLoopThreadPool.cpp
        Logger.cpp
        Poller.cpp
        Thread.cpp
        Timestamp.cpp
        Socket.cpp
        InetAddress.cpp
        Acceptor.cpp
)

# Build shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})
