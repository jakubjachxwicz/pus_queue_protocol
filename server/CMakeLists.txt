cmake_minimum_required(VERSION 3.28.3)
project(pus_queue_protocol C)

set(CMAKE_C_STANDARD 11)

include_directories(message_handlers)

add_executable(pus_queue_protocol
        message_handlers/handlers.h
        message_handlers/hello.c
        main.c
        types.h
        message_handlers/common.c)

target_link_libraries(pus_queue_protocol ssl crypto pthread)
