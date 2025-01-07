cmake_minimum_required(VERSION 3.15)
project(simple_process_monitor VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_compile_options(-Wall -Wextra -Werror)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "include/*.h" "src/*.h" "src/*.c" "src/*.cpp")

add_library(simple_process_monitor SHARED ${SRC_FILES})

target_compile_options(simple_process_monitor
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wconversion -Wno-unused-parameter -Wold-style-cast -Woverloaded-virtual -Wshadow>
)

target_include_directories(simple_process_monitor PUBLIC include)

find_package(Threads REQUIRED)
target_link_libraries(simple_process_monitor PUBLIC Threads::Threads)

add_executable(busy_loop "test/busy_loop.cpp")
target_link_libraries(busy_loop PRIVATE Threads::Threads)

add_executable(test "test/test.cpp")
target_link_libraries(test PRIVATE simple_process_monitor)

include(cmake/scanners.cmake)
