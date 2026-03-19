aux_source_directory(. SRC_LIST)
aux_source_directory(../common COMMON_LIST)



add_executable(chat_client ${SRC_LIST} ${COMMON_LIST})

target_link_libraries(chat_client
    pthread
)