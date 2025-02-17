cmake_minimum_required(VERSION 3.10)
project(muduo_modernCpp VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找必要的包
find_package(Threads REQUIRED)

# 创建库目标
add_library(muduo_base
    Thread.cpp
)

# 设置头文件包含目录
target_include_directories(muduo_base
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接必要的库
target_link_libraries(muduo_base
    PRIVATE
        Threads::Threads
)
