cmake_minimum_required(VERSION 3.10)
project(VoiceInteraction VERSION 1.0.0 LANGUAGES CXX)

# C++ Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# package management
find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)

set(SHERPA_HOME "${CMAKE_SOURCE_DIR}/../SpeakerTest/model/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu")
if(NOT EXISTS "${SHERPA_HOME}")
    message(FATAL_ERROR "Cant find Sherpa Lib: ${SHERPA_HOME} \nPlease Check the name of menu!")
endif()
set(VOSK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk")
if(NOT EXISTS "${VOSK_LIB_PATH}")
    message(FATAL_ERROR "Cant find Sherpa Lib: ${VOSK_LIB_PATH} \nPlease Check the name of menu!")
endif()

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest"       
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk"  
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest"          
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools"
    ${SHERPA_HOME}/include               
)

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk")
link_directories(${SHERPA_HOME}/lib)

add_executable(VoiceInteraction
    main.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/MicrophoneEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest/SpeakerEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/VisonTools.cpp"
)

target_link_libraries(VoiceInteraction
    PRIVATE
    sherpa-onnx-c-api
    onnxruntime
    ALSA::ALSA       #speaker
    Threads::Threads #multithreading
    vosk             #microphone
    #espeak-ng        #speaker
)

set(VOSK_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/model/vosk")
set(SHERPA_RPATH "${SHERPA_HOME}/lib")

set_target_properties(VoiceInteraction PROPERTIES 
    BUILD_RPATH "${VOSK_RPATH};${SHERPA_RPATH}"
    INSTALL_RPATH "${VOSK_RPATH};${SHERPA_RPATH}"
    BUILD_WITH_INSTALL_RPATH TRUE
)