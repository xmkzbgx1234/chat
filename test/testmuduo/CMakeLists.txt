# 完整编译命令：g++ -g muduo_server.cpp -o server -lmuduo_base -lmuduo_net -lpthread

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 配置头文件搜索路径
# include_directories(${PROJECT_SOURCE_DIR}/include)
# 配置库文件搜索路径
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# 设置需要编译的源文件列表
set(SOURCES
    muduo_server.cpp
)

# 把.指定路径下的所有源文件加入到编译列表中
# aux_source_directory(. SOURCES)

add_executable(server ${SOURCES})

# 链接需要的库文件
target_link_libraries(server
    muduo_net
    muduo_base
    pthread
)
