cmake_minimum_required(VERSION 3.7.0)

project(opencv-camera-calback VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(CameraTest
  main.cpp
  CameraEngine.cpp
  YoloDetector.cpp
)

target_link_libraries(CameraTest ${OpenCV_LIBS} Threads::Threads)

install(TARGETS CameraTest)
