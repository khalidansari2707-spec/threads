cmake_minimum_required(VERSION 3.15)
project(MultiCoreX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization flags
if(MSVC)
    add_compile_options(/W4 /O2 /EHsc /utf-8)
else()
    add_compile_options(-Wall -Wextra -O3 -pthread)
endif()

find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/include
)

set(CORE_SOURCES
    backend/src/thread_pool.cpp
    backend/src/benchmarks.cpp
    backend/src/synchronization.cpp
    backend/src/task_manager.cpp
    backend/src/api_server.cpp
)

add_library(multicorex_core STATIC ${CORE_SOURCES})

target_link_libraries(multicorex_core PUBLIC
    Threads::Threads
)

if(WIN32)
    target_link_libraries(multicorex_core PUBLIC
        ws2_32
        crypt32
    )
endif()

# Main Application Executable
add_executable(multicorex_server backend/src/main.cpp)
target_link_libraries(multicorex_server PRIVATE multicorex_core)

# Tests Subdirectory
enable_testing()
add_subdirectory(tests)
