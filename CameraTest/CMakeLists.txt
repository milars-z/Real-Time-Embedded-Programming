cmake_minimum_required(VERSION 3.7.0)

project(opencv-camera-calback VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

add_executable(CameraTest
  main.cpp
  CameraEngine.cpp
  AttentionDetector.cpp
  FeatureManager.cpp
)

target_link_libraries(CameraTest 
    Qt5::Widgets
    # qwt-qt5          
    Threads::Threads 
    atomic
    ${OpenCV_LIBS}    
)

install(TARGETS CameraTest)
