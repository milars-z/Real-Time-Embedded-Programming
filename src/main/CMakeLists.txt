cmake_minimum_required(VERSION 3.10)
project(main VERSION 1.0.0 LANGUAGES CXX)

# --- C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Package Management ---
find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)
find_package(nlohmann_json 3 REQUIRED)

# --- Executable ---
add_executable(main
    main.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../Microphone/MicrophoneEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Speaker/SpeakerEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/VisonTools.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction/nlu_test.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction/voiceInteraction.cpp" 
)

target_compile_definitions(main PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../"
)

# --- Path Definitions & Checks ---

# 1. Sherpa (TTS)
set(SHERPA_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../Speaker/model/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu")
if(NOT EXISTS "${SHERPA_HOME}")
    message(FATAL_ERROR "Can't find Sherpa Home: ${SHERPA_HOME}")
endif()

# 2. Vosk (ASR)
set(VOSK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Microphone/model/vosk")
if(NOT EXISTS "${VOSK_LIB_PATH}")
    message(FATAL_ERROR "Can't find Vosk Lib Path: ${VOSK_LIB_PATH}")
endif()

# 3. OnnxRuntime
set(ORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction/onnxruntime")
if(NOT EXISTS "${ORT_ROOT}")
    message(FATAL_ERROR "Can't find OnnxRuntime Root: ${ORT_ROOT}")
endif()

set(SHERPA_LIB "${SHERPA_HOME}/lib/libsherpa-onnx-c-api.so")
set(VOSK_LIB   "${VOSK_LIB_PATH}/libvosk.so")
set(ORT_LIB    "${ORT_ROOT}/lib/libonnxruntime.so")

# change include_xx to target_include_xx
target_include_directories(main PRIVATE
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../main"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Microphone"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Microphone/model/vosk"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Speaker"

    "${SHERPA_HOME}/include"
    "${ORT_ROOT}/include" 
)

target_link_directories(main PRIVATE
    "${SHERPA_HOME}/lib"
    "${ORT_ROOT}/lib"
    "${VOSK_LIB_PATH}"
)

target_link_libraries(main PRIVATE
    sherpa-onnx-c-api
    onnxruntime
    vosk
    nlohmann_json::nlohmann_json
    ALSA::ALSA
    Threads::Threads
    dl
)

# --- RPATH Settings ---
set(MY_RPATH_DIRS 
    "${VOSK_LIB_PATH}" 
    "${SHERPA_HOME}/lib" 
    "${ORT_ROOT}/lib"
)

#RPATH
set_target_properties(main PROPERTIES 
    BUILD_RPATH "${MY_RPATH_DIRS}"    
    INSTALL_RPATH "${MY_RPATH_DIRS}"  
    BUILD_WITH_INSTALL_RPATH FALSE   
)