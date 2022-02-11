cmake_minimum_required(VERSION 3.15)
project(xtr CXX)

set(XTRCTL_TARGET xtrctl)

option(BUILD_BENCHMARK "Build benchmark test" ON)
option(BUILD_SINGLE_HEADER "Build header-only" ON)
option(INSTALL_DOCS "Install man pages" ON)
option(ENABLE_EXCEPTIONS "Enable the use of exceptions" ON)
option(ENABLE_LTO "Enable link-time optimisation" ON)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

file(GLOB_RECURSE HEADER_FILES include/*.hpp)

add_library(${PROJECT_NAME} src/command_dispatcher.cpp
                            src/command_path.cpp
                            src/consumer.cpp
                            src/fd_storage.cpp
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
target_link_libraries(${PROJECT_NAME} fmt::fmt Threads::Threads)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES}")
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_definitions(${PROJECT_NAME} PRIVATE XTR_FUNC=)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wconversion -Wshadow -Wcast-qual -Wformat=2 -pedantic -pipe)
endif()

if (NOT ENABLE_EXCEPTIONS)
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
endif()

if (ENABLE_LTO)
    target_compile_options(${PROJECT_NAME} PRIVATE -flto)
    if (CMAKE_CXX_COMPILER_ID MATCHES ".*GNU")
        find_program(GCC_AR gcc-ar)
        if (GCC_AR)
            set(CMAKE_AR ${GCC_AR})
        endif()
        find_program(GCC_RANLIB gcc-ranlib)
        if (GCC_RANLIB)
            set(CMAKE_RANLIB ${GCC_RANLIB})
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        include(llvm-toolchain)
    endif()
endif()

add_executable(${XTRCTL_TARGET} src/xtrctl.cpp)
target_link_libraries(${XTRCTL_TARGET} ${PROJECT_NAME})

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} ${XTRCTL_TARGET}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(BUILD_SINGLE_HEADER)
    execute_process(COMMAND bash -c scripts/make_single_include.sh
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    install(FILES single_include/xtr/logger.hpp
            DESTINATION single_include/xtr)
endif()

if(INSTALL_DOCS)
    install(FILES docs/xtrctl.1
            DESTINATION man/man1)
    install(FILES docs/libxtr.3 docs/libxtr-quickstart.3 docs/libxtr-userguide.3
            DESTINATION man/man3)
endif()
