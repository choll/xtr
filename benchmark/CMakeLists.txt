cmake_minimum_required(VERSION 3.1)
project(xtr_benchmark CXX)

find_package(benchmark REQUIRED)

add_executable(${PROJECT_NAME}  logger.cpp
                                main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} ${CMAKE_PROJECT_NAME} benchmark::benchmark)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})