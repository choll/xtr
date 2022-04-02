cmake_minimum_required(VERSION 3.1)
project(xtr_test CXX)

find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME}  align.cpp
                                command_client.cpp
                                command_dispatcher.cpp
                                fd_storage.cpp
                                file_descriptor.cpp
                                logger.cpp
                                main.cpp
                                memory_mapping.cpp
                                mirrored_memory_mapping.cpp
                                pagesize.cpp
                                synchronized_ring_buffer.cpp
                                throw.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} ${CMAKE_PROJECT_NAME} Catch2::Catch2 dl)

if (EXISTS /usr/lib/x86_64-linux-gnu/liburing.so)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_PROJECT_NAME} uring)
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
