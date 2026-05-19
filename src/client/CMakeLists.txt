set(CLIENT_SRC_LIST
    main.cc
    clientApp.cc
    clientCommandDispatcher.cc
    clientSession.cc
    sqliteStorage.cc
)

aux_source_directory(../common COMMON_LIST)

add_executable(chat_client ${CLIENT_SRC_LIST} ${COMMON_LIST})

target_link_libraries(chat_client
    sqlite3
    pthread
)
