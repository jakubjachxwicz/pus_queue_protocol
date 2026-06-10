cmake_minimum_required(VERSION 3.28.3)
project(pus_kiosk_client C)

set(CMAKE_C_STANDARD 11)

set(CLIENTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(kiosk_client
        kiosk_client.c
        ${CLIENTS_ROOT}/clients/handlers/hello.c
        ${CLIENTS_ROOT}/clients/handlers/register.c
        ${CLIENTS_ROOT}/clients/common/client_common.c)

target_include_directories(kiosk_client PRIVATE ${CLIENTS_ROOT})

target_link_libraries(kiosk_client ssl crypto)
