cmake_minimum_required(VERSION 3.10)
project(main VERSION 1.0.0 LANGUAGES CXX)

# --- C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ServoEventControl)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wconversion -Wextra -pedantic)

find_package(Threads REQUIRED)

add_executable(servo_event PwmBoardController.cpp)

target_link_libraries(servo_event PRIVATE Threads::Threads)
