cmake_minimum_required(VERSION 3.1)
project(xtr CXX)

set(XTRCRL_TARGET xtrcrl)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

file(GLOB_RECURSE HEADER_FILES include/*.hpp)

add_library(${CMAKE_PROJECT_NAME}   src/command_dispatcher.cpp
                                    src/command_path.cpp
                                    src/consumer.cpp
                                    src/file_descriptor.cpp
                                    src/logger.cpp
                                    src/log_level.cpp
                                    src/matcher.cpp
                                    src/memory_mapping.cpp
                                    src/mirrored_memory_mapping.cpp
	                                src/pagesize.cpp
                                    src/regex_matcher.cpp
                                    src/sink.cpp
	                                src/throw.cpp
                                    src/tsc.cpp
                                    src/wildcard_matcher.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} fmt::fmt Threads::Threads)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 20
                                            PUBLIC_HEADER "${HEADER_FILES}")
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC include)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE XTR_FUNC=)

add_executable(${XTRCRL_TARGET} src/xtrctl.cpp)
target_link_libraries(${XTRCRL_TARGET} ${CMAKE_PROJECT_NAME})

install(TARGETS ${CMAKE_PROJECT_NAME} ${XTRCRL_TARGET}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
endif()
