cmake_minimum_required(VERSION 3.16)
project(log_filter_optimize VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认优化等级与手工编译命令保持一致
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

# 原始基线（未改动，仅供对照）
add_executable(baseline src/baseline.cpp)
target_include_directories(baseline PRIVATE src)
target_compile_options(baseline PRIVATE -O2)

# 重构版：抽取 filter_records 接口，行为与基线一致
add_executable(refactored src/refactored.cpp)
target_include_directories(refactored PRIVATE src)
target_compile_options(refactored PRIVATE -O2)

# 候选 A：无分支筛选
add_executable(branchless src/branchless.cpp)
target_include_directories(branchless PRIVATE src)
target_compile_options(branchless PRIVATE -O2)

# 候选 B：多线程 + 两遍扫描 + 排他前缀和
add_executable(parallel src/parallel.cpp)
target_include_directories(parallel PRIVATE src)
target_compile_options(parallel PRIVATE -O2)
target_link_libraries(parallel PRIVATE Threads::Threads)
