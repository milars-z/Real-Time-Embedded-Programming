cmake_minimum_required(VERSION 3.10)
project(VoiceInteraction VERSION 1.0.0 LANGUAGES CXX)

# --- C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Package Management ---
find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)
find_package(nlohmann_json 3 REQUIRED)

# --- Path Definitions & Checks ---

# 1. Sherpa (TTS)
set(SHERPA_HOME "${CMAKE_SOURCE_DIR}/../SpeakerTest/model/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu")
if(NOT EXISTS "${SHERPA_HOME}")
    message(FATAL_ERROR "Can't find Sherpa Home: ${SHERPA_HOME}")
endif()

# 2. Vosk (ASR)
set(VOSK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk")
if(NOT EXISTS "${VOSK_LIB_PATH}")
    message(FATAL_ERROR "Can't find Vosk Lib Path: ${VOSK_LIB_PATH}")
endif()

# 3. OnnxRuntime
set(ORT_ROOT "${CMAKE_SOURCE_DIR}/onnxruntime")
if(NOT EXISTS "${ORT_ROOT}")
    message(FATAL_ERROR "Can't find OnnxRuntime Root: ${ORT_ROOT}")
endif()

# --- Include Directories  ---
include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest"       
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk"  
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest"          
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools"
    "${SHERPA_HOME}/include"
    "${ORT_ROOT}/include"  
)

# --- Link Directories  ---
link_directories(
    "${VOSK_LIB_PATH}"
    "${SHERPA_HOME}/lib"
    "${ORT_ROOT}/lib"      
)

# --- Executable ---
add_executable(VoiceInteraction
    main.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/MicrophoneEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest/SpeakerEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/VisonTools.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction/nlu_test.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoiceInteraction/voiceInteraction.cpp" 
)

# --- Linking ---
target_link_libraries(VoiceInteraction
    PRIVATE
    sherpa-onnx-c-api
    onnxruntime
    nlohmann_json::nlohmann_json
    ALSA::ALSA       
    Threads::Threads 
    vosk             
    dl
)

# --- RPATH Settings ---
set(MY_RPATH_DIRS 
    "${VOSK_LIB_PATH}" 
    "${SHERPA_HOME}/lib" 
    "${ORT_ROOT}/lib"
)

#RPATH
set_target_properties(VoiceInteraction PROPERTIES 
    BUILD_RPATH "${MY_RPATH_DIRS}"    
    INSTALL_RPATH "${MY_RPATH_DIRS}"  
    BUILD_WITH_INSTALL_RPATH FALSE   
)