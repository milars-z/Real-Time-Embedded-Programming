cmake_minimum_required(VERSION 3.10)
project(ScreenTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${OpenCV_INCLUDE_DIRS} )

find_package(SDL2 REQUIRED)
find_package(OpenCV REQUIRED)


add_library(lvgl_lib STATIC IMPORTED)
set_target_properties(lvgl_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lvgl_lib/liblvgl.a"
)

add_executable(ScreenTest
    main.cpp
    Screen_ui.cpp
)

target_include_directories(ScreenTest PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
)

target_link_libraries(ScreenTest PRIVATE 
    lvgl_lib  
    ${SDL2_LIBRARIES}
    ${OpenCV_LIBS}
    pthread
)