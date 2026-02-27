cmake_minimum_required(VERSION 3.10)
project(SpeakerDriver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)

set(SHERPA_HOME "${CMAKE_SOURCE_DIR}/model/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu")

if(NOT EXISTS "${SHERPA_HOME}")
    message(FATAL_ERROR "Cant find Sherpa Lib: ${SHERPA_HOME} \nPlease Check the name of menu！")
endif()

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHERPA_HOME}/include
)

link_directories(${SHERPA_HOME}/lib)

add_executable(SpeakerTest
    main.cpp
    SpeakerEngine.cpp
    ../Tools/VisonTools.cpp
)

target_link_libraries(SpeakerTest
    PRIVATE
    sherpa-onnx-c-api
    onnxruntime
    ALSA::ALSA
    Threads::Threads
    # espeak-ng   
)

set_target_properties(SpeakerTest PROPERTIES
    BUILD_RPATH "${SHERPA_HOME}/lib"
    INSTALL_RPATH "${SHERPA_HOME}/lib"
)