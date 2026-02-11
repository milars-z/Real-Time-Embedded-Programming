cmake_minimum_required(VERSION 3.10)
project(SpeakerDriver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

# 包含路径
include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(SpeakerTest
    main.cpp
    SpeakerEngine.cpp
)

target_link_libraries(SpeakerTest
    PRIVATE
    ALSA::ALSA
    Threads::Threads
    espeak-ng   # 链接 TTS 引擎
)