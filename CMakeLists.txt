cmake_minimum_required(VERSION 3.10)

project(MyProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# 添加调试符号
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 默认使用Debug模式
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 配置最终生成的库文件和可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/client)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/service)
include_directories(${PROJECT_SOURCE_DIR}/include/common)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/include/server/utils)
include_directories(${PROJECT_SOURCE_DIR}/include/server/game)

# 指定搜索的子目录
add_subdirectory(src)

