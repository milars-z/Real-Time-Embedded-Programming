cmake_minimum_required(VERSION 3.10)
project(MicrophoneTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vosk
)


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/vosk)

add_executable(MicrophoneTest
    main.cpp
    MicrophoneEngine.cpp
)

target_link_libraries(MicrophoneTest
    PRIVATE
    ALSA::ALSA
    Threads::Threads
    vosk 
)

set_target_properties(MicrophoneTest PROPERTIES 
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/vosk"
    BUILD_WITH_INSTALL_RPATH TRUE
)