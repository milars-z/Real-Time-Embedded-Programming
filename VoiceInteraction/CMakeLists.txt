cmake_minimum_required(VERSION 3.10)
project(VoiceInteraction VERSION 1.0.0 LANGUAGES CXX)

# C++ Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# package management
find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)

include_directories(
    ${ALSA_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest"       
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/vosk"  
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest"          
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools"               
)

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/vosk")

add_executable(VoiceInteraction
    main.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/MicrophoneEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../SpeakerTest/SpeakerEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/VisonTools.cpp"
)

target_link_libraries(VoiceInteraction
    PRIVATE
    ALSA::ALSA       #speaker
    Threads::Threads #multithreading
    vosk             #microphone
    espeak-ng        #speaker
)

set_target_properties(VoiceInteraction PROPERTIES 
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/../MicrophoneTest/vosk"
    BUILD_WITH_INSTALL_RPATH TRUE
)