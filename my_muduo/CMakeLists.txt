cmake_minimum_required(VERSION 3.10)
project(my_muduo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11) # Muduo 原版是 C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. 查找依赖 ---
# 网络库通常需要多线程支持 (pthread)
find_package(Threads REQUIRED)

# --- 2. 搜集源文件 ---
# 搜集 src 下的所有 cpp
file(GLOB LIB_SOURCES "src/*.cpp")

# --- 3. 生成库文件 ---
# 将核心代码编译为静态库 (也可以选 SHARED 生成动态库)
add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

# --- 4. 指定头文件搜索路径 ---
# PUBLIC 意味着：
# 1. 编译这个库时，去 include/ 找头文件
# 2. 别人链接这个库时，也会自动把 include/ 加入搜索路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- 5. 链接库依赖 ---
# 链接 pthread 库
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

# --- 6. (可选) 编译示例程序 ---
# 如果你有测试用的 main.cpp，可以放在 example 目录下
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/example/test_server.cpp")
    add_executable(test_server example/test_server.cpp)
    # 让测试程序链接我们刚刚生成的 my_muduo 库
    target_link_libraries(test_server PRIVATE ${PROJECT_NAME})
endif()