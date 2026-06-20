cmake_minimum_required(VERSION 3.10)
project(my_muduo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. 构建模式优化
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 2. 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

# 3. C++ 标准
set(CMAKE_CXX_STANDARD 11) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 直接在 Release 模式下开启常用的极致优化参数
# -O3: 优化等级
# -DNDEBUG: 关闭 assert
# -march=native: 针对当前 CPU 优化
# -flto: 开启链接时优化 (这是手动开启 LTO，不需要那个报错的模块)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -march=native -flto")
endif()

# 4. 查找依赖
find_package(Threads REQUIRED)

# 5. 搜集源文件
file(GLOB LIB_SOURCES "src/*.cpp")

# 6. 生成库
add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

# 7. 指定头文件
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 8. 链接
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

# 9. 编译示例/压测程序 (确保路径正确)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/example/test_server.cpp")
    add_executable(test_server example/test_server.cpp)
    target_link_libraries(test_server PRIVATE ${PROJECT_NAME})
endif()