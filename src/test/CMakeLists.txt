# cmake_minimum_required(VERSION 3.20)
# project(KeypressPublisherSubscriber LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_compile_options(-Wall -Wconversion -Wextra -pedantic)
# find_package(Threads REQUIRED)

# add_executable(my_timer multi-timer.cpp)
# target_link_libraries(my_timer Threads::Threads)

cmake_minimum_required(VERSION 3.20)
project(KeypressPublisherSubscriber LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wconversion -Wextra -pedantic)
endif()

find_package(Threads REQUIRED)

add_executable(my_timer multi-timer.cpp)

target_link_libraries(my_timer PRIVATE Threads::Threads)

if(MINGW)
    target_link_options(my_timer PRIVATE -static-libgcc -static-libstdc++ -static)
endif()