cmake_minimum_required(VERSION 3.28.3)
project(pus_queue_protocol C)

set(CMAKE_C_STANDARD 11)

include_directories(message_handlers)

add_executable(pus_queue_protocol
        message_handlers/handlers.h
        message_handlers/hello.c
        main.c
        types.h
        message_handlers/common.c
        queue/queue.h
        queue/queue.c
        message_handlers/register.c
        message_handlers/subscribe.c
        message_handlers/status.c
        message_handlers/ping.c
        message_handlers/bye.c
        registry.c
        notify.h
        notify.c
        message_handlers/notify_ack.c)

target_link_libraries(pus_queue_protocol ssl crypto pthread)
