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
if (CMAKE_SYSTEM MATCHES "Linux")
    find_package(liburing)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU")
    add_compile_options(-Wall -Wextra -Wconversion -Wshadow -Wcast-qual -Wformat=2 -pedantic -pipe)
endif()

if (NOT ENABLE_EXCEPTIONS)
    add_compile_options(-fno-exceptions)
endif()

if (ENABLE_LTO)
    # CMake's INTERPROCEDURAL_OPTIMIZATION feature isn't used as it does not
    # use gcc-ranlib and gcc-ar (as recommended by
    # https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html).
    add_compile_options(-flto)
    add_link_options(-flto)
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
        find_program(LLVM_AR llvm-ar)
        if (LLVM_AR)
            set(CMAKE_AR ${LLVM_AR})
        endif()
        find_program(LLVM_RANLIB llvm-ranlib)
        if (LLVM_RANLIB)
            set(CMAKE_RANLIB ${LLVM_RANLIB})
        endif()
    endif()
endif()

file(GLOB_RECURSE HEADER_FILES include/*.hpp)

add_library(${PROJECT_NAME} src/buffer.cpp
                            src/command_dispatcher.cpp
                            src/command_path.cpp
                            src/consumer.cpp
                            src/fd_storage_base.cpp
                            src/fd_storage.cpp
                            src/file_descriptor.cpp
                            src/io_uring_fd_storage.cpp
                            src/logger.cpp
                            src/log_level.cpp
                            src/matcher.cpp
                            src/memory_mapping.cpp
                            src/mirrored_memory_mapping.cpp
                            src/pagesize.cpp
                            src/posix_fd_storage.cpp
                            src/regex_matcher.cpp
                            src/sink.cpp
                            src/throw.cpp
                            src/tsc.cpp
                            src/wildcard_matcher.cpp)
target_link_libraries(${PROJECT_NAME} fmt::fmt Threads::Threads)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_definitions(${PROJECT_NAME} PRIVATE XTR_FUNC=)

add_executable(${XTRCTL_TARGET} src/xtrctl.cpp)
target_link_libraries(${XTRCTL_TARGET} ${PROJECT_NAME})

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} ${XTRCTL_TARGET}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

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
