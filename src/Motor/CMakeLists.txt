cmake_minimum_required(VERSION 3.20)

project(ServoEventControl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wconversion -Wextra -pedantic)

find_package(Threads REQUIRED)

add_executable(servo_event servo_event_driven.cpp)

target_link_libraries(servo_event PRIVATE Threads::Threads)
