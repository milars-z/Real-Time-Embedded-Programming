cmake_minimum_required(VERSION 3.10)
project(main VERSION 1.0.0 LANGUAGES CXX)

# --- C++ Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Package Management ---
find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)
find_package(nlohmann_json 3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(SDL2 REQUIRED)

add_definitions(-DTESTMODE)
add_definitions(-DPRECACHE)

# --- Executable ---
add_executable(main
    "main.cpp"

    # brain
    "${CMAKE_CURRENT_SOURCE_DIR}/src/brain/cogniArm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/brain/NluHandle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/brain/RobotBrain.cpp"

    # driver
    "${CMAKE_CURRENT_SOURCE_DIR}/src/driver/camera/AttentionDetector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/driver/camera/CameraHandle.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/driver/camera/FeatureManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/driver/motion/MotionManager.cpp"

    # engines
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engines/camera/CameraEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engines/microphone/MicrophoneEngine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engines/motion/PwmBoardController.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/engines/speaker/SpeakerEngine.cpp"

    # executors
    "${CMAKE_CURRENT_SOURCE_DIR}/src/executors/camera/CameraApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/executors/motion/MotionApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/executors/speaker/SpeakerApp.cpp"

    # producers 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/producers/microphone/MicrophoneApp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/producers/ui/ScreenApp.cpp"

    #supervisor
    "${CMAKE_CURRENT_SOURCE_DIR}/src/supervisor/TaskSupervisor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/supervisor/TaskMonitor.cpp"
    

    #Kinematics
    "${CMAKE_CURRENT_SOURCE_DIR}/src/kinematics/CoordinateTransformer.cpp"

    #system
     "${CMAKE_CURRENT_SOURCE_DIR}/src/system/system_config.cpp"

    # ui 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Screen_ui.cpp"

    # utils 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Tools.cpp"

)

target_compile_definitions(main PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_library(lvgl_lib STATIC IMPORTED)
set_target_properties(lvgl_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvgl_lib/liblvgl.a"
)


# --- Path Definitions & Checks ---

# 1. Sherpa (TTS)
set(SHERPA_HOME "${CMAKE_CURRENT_SOURCE_DIR}/third_party/speaker/model/sherpa-onnx-v1.12.25-linux-aarch64-shared-cpu")
if(NOT EXISTS "${SHERPA_HOME}")
    message(FATAL_ERROR "Can't find Sherpa Home: ${SHERPA_HOME}")
endif()

# 2. Vosk (ASR)
set(VOSK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microphone/model/vosk")
if(NOT EXISTS "${VOSK_LIB_PATH}")
    message(FATAL_ERROR "Can't find Vosk Lib Path: ${VOSK_LIB_PATH}")
endif()

# 3. OnnxRuntime
set(ORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime")
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
    ${OpenCV_INCLUDE_DIRS} 
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/microphone/model/vosk"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lvgl"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/brain"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/driver/camera"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/driver/motion"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/engines/camera"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/engines/microphone"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/engines/motion"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/engines/speaker"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/executors"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/executors/camera"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/executors/motion"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/executors/speaker"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/producers/microphone"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/producers/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/utils"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/supervisor"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/kinematics"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/system"
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
    lvgl_lib
    ${OpenCV_LIBS} 
    ${SDL2_LIBRARIES}
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