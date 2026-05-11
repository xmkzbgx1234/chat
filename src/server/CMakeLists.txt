
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(../common COMMON_LIST)
aux_source_directory(./redis REDIS_LIST)
aux_source_directory(./service SERVICE_LIST)
aux_source_directory(./utils UTILS_LIST)

add_executable(chat_server ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} 
                            ${COMMON_LIST} ${REDIS_LIST} ${SERVICE_LIST} 
                            ${UTILS_LIST}) 

target_link_libraries(chat_server
    muduo_net
    muduo_base
    mysqlclient
    hiredis
    pthread
    sodium
)