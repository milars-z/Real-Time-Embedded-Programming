#include "cogniArm.hpp"

#include "SpeakerApp.hpp"
#include "MotionApp.hpp"
#include "CameraApp.hpp"
#include "RobotBrain.hpp"
#include "ScreenApp.hpp"
#include "MicrophoneApp.hpp" 
#include "Config.hpp" 
#include "Tools.hpp"    
#include "TaskMonitor.hpp" 
#include "TaskSupervisor.hpp"
#include "SystemCode.hpp"

#include <iostream>
#include <unistd.h> 

extern std::atomic<bool> _exit_signal;

RobotSystem::RobotSystem() = default;

RobotSystem::~RobotSystem() = default;


// System initialization
// Input:
//      cfg: system configuration
//           See system_config.hpp for detailed configuration settings
// Output:
//      true  : initialization succeeded
//      false : failed to find the speaker and microphone hardware
// The initialization status of subsequent components is reflected by state in start()
bool RobotSystem::init(SystemConfig cfg) {
    std::cout << "[CogniArm] Initializing hardware..." << std::endl;

    std::string speaker_path = "";
    std::string mic_path = "";
    bool hardware_check_passed = true;
    std::string speaker_text = Config::Speaker::SPEAKER_TEXT;

    sys_cfg = cfg;
    
    // Hardware detection
    if( sys_cfg.enableSpeaker ){
        speaker_path = find_alsa_device(Config::Hardware::SPEAKER_NAME);
        if (speaker_path.empty()){
            std::cerr << "[Fatal][CogniArm] Audio hardware device not found(Speaker)!" << std::endl;
            hardware_check_passed = false;
        }
        
    }
    
    if( sys_cfg.enableMicrophone ) {
        mic_path = find_alsa_device(Config::Hardware::MIC_NAME);
        if (mic_path.empty()){
            std::cerr << "[Fatal][CogniArm] Audio hardware device not found(Microphone)!" << std::endl;
            hardware_check_passed = false;
        }
    }

    if(!hardware_check_passed){
        std::cerr << "[Fatal][CogniArm] Speaker Hardware path detection failed. Please refer to the README to find the correct path." << std::endl;
        return false;
    }

    // Global task monitor
    globalMonitor = std::make_shared<TaskMonitor>();

    // Initialize the execution layer
    if( sys_cfg.enableSpeaker ){
        std::cout << "[Init][CogniArm] Initializing Speaker..." << std::endl;
        speaker = std::make_shared<SpeakerExecutor>(state,speaker_path,speaker_text,globalMonitor);
    }
    
    if( sys_cfg.enableMotion){
        std::cout << "[Init][CogniArm] Initializing Motion..." << std::endl;
        motion  = std::make_shared<MotionExecutor>(state,globalMonitor);
    }
    
    if( sys_cfg.enableCamera){
        std::cout << "[Init][CogniArm] Initializing Camera..." << std::endl;
        camera  = std::make_shared<CameraExecutor>(state,globalMonitor);
    }
    
    // Initialize the brain logic layer
    std::cout << "[Init][CogniArm] Initializing Brain..." << std::endl;
    brain = std::make_shared<RobotBrain>(speaker, motion, camera, globalMonitor);

    // Initialize the input layer and bind input callbacks to the brain
    // Voice input
    if( sys_cfg.enableMicrophone){
        std::cout << "[Init][CogniArm] Initializing Microphone..." << std::endl;
        voiceIn = std::make_unique<VoiceProducer>(state, mic_path, [this](std::string text) {
            if(brain) brain->handleIncomingText(text);
    }, globalMonitor);
    }
    

    // Screen input
    if( sys_cfg.enableScreen){
        std::cout << "[Init][CogniArm] Initializing Screen..." << std::endl;
        screenIn = std::make_unique<ScreenProducer>(camera, [this](std::string t, std::string d){
        if(brain) brain->handleUISignal(t, d);
    }, globalMonitor);
    }
    
    // supervisor, used to provide feedback and record timing information
    std::cout << "[Init][CogniArm] Initializing Supervisor..." << std::endl;
    supervisor = std::make_unique<TaskSupervisor>(globalMonitor,motion,speaker,brain,sys_cfg);

    return true;
}


// System startup
// Input: -
// Output: -
// Main loop
// 1 - Start all threads
// 2 - Check whether state meets the startup requirements; exit directly if not
// 3 - Start the main loop and refresh the screen (in speaker-test mode, a task is sent to the speaker every 2 seconds)
void RobotSystem::start() {
    if (isRunning) return;

    _exit_signal = false;
    isRunning = true;

    std::cout << "[Init][CogniArm] Modular system started." << std::endl;

    start_thread();

    if(!check_state(state)){
        stop();
    } 
    else{
        print_startup_banner(sys_cfg);
    }

    std::cout << "[Init][CogniArm] Initialization complete." << std::endl;

    // speaker test
    if(speaker && (!screenIn) && (!voiceIn)){
        Init_speaker_test();
    }

    while (isRunning) {

        if (_exit_signal) {
            break;
        }

        // UI update loop
        if(screenIn){
            uint32_t next_ms = screenIn->update();
            // Limit the frame rate to approximately 33 FPS
            next_ms = std::max(1u, std::min(next_ms, 30u));
            usleep(next_ms * 1000);
        }

        // Speaker single-module test
        if(speaker && (!screenIn) && (!voiceIn)){

            if (current_line_idx < test_scripts.size()) {
                std::string text_to_say = test_scripts[current_line_idx];
                std::cout << text_to_say << std::endl;      
                speaker->pushTask(text_to_say);
            }
                current_line_idx++;
                usleep(2000000);
        }
    }
    stop();
}

// System startup
// Input: -
// Output: -
// Start threads
// Task threads -- Executor layer -- Producer layer -- Supervisor layer
void RobotSystem::start_thread(){

    // Start the low-level executor threads first: Callback -> TaskQueue
    std::cout << "[Init][CogniArm] Starting executor threads..." << std::endl;
    if (speaker) speaker->_start(3);
    if (motion)  motion->_start(1);
    if (camera)  camera->_start(2);

    // Start task threads: TaskQueue -> Executor module
    std::cout << "[Init][CogniArm] Starting task threads..." << std::endl;
    if(speaker) speaker->start();
    if(motion) motion->start();
    if(camera) camera->start();

    // Bind threads
    std::cout << "[Init][CogniArm] Binding task threads..." << std::endl;
    if(speaker) speaker->pinThread(3);
    if(motion) motion->pinThread(1);
    if(camera) camera->pinThread(2);

    // Start producer threads after the executor threads are ready
    if(voiceIn)  voiceIn->start();
    if(screenIn) screenIn->start(state);
    
    std::cout << "[Init][CogniArm] Starting producer threads..." << std::endl;
    if(voiceIn) voiceIn->_start(3);

    // Start the monitoring thread
    supervisor->start_thread(0);

}

// System shutdown
// Input: -
// Output: -
// Stop producer threads -- stop task threads from accepting tasks -- stop executor threads from processing tasks
void RobotSystem::stop() {
    if (!isRunning) return;
    isRunning = false;

    std::cout << "[End][CogniArm] Stopping system..." << std::endl;

    supervisor->stop_thread();
    
    // Stop input tasks
    if (voiceIn) voiceIn->stop();
    if (screenIn) screenIn->stop();

    // Stop output tasks
    // Exit task threads
    if (motion) motion->stop();
    if (speaker) speaker->stop();
    if (camera) camera->stop();

    // Exit low-level threads
    if (motion) motion->_stop();
    if (speaker) speaker->_stop();
    if (camera) camera->_stop();

    std::cout << "[End][CogniArm] Threads exited safely... destroying resources..." << std::endl;
}

// System state check
// Input: state
// Output: true -- the system can enter normally
//         false -- requirements are not met, and the system exits
// Detailed error logs should be refined later
bool RobotSystem::check_state(std::atomic<int>& state){

    int current_state = state;

    if (current_state == 0) {
        
        return true;
    } 

    const int FATAL_ERRORS = ERR_SPEAKER_INIT | ERR_MOTION_INIT | ERR_CAMERA_INIT | ERR_SCREEN_INIT;

    if (current_state & FATAL_ERRORS) {
        std::cerr << "[Fatal] Critical hardware error, status code: " << current_state << std::endl;
        if (current_state & ERR_SPEAKER_INIT) std::cerr << " [Error][CogniArm]" << " -> speaker failure" << std::endl;
        if (current_state & ERR_MOTION_INIT)  std::cerr << " [Error][CogniArm]" << " -> motion failure" << std::endl;
        if (current_state & ERR_CAMERA_INIT)  std::cerr << " [Error][CogniArm]" << " -> camera failure" << std::endl;
        if (current_state & ERR_MIC_INIT)     std::cerr << " [Error][CogniArm]" << " -> microphone failure" << std::endl;
        return false; 
    }

    if (current_state & ERR_SCREEN_INIT) {
        std::cerr << "[Error][CogniArm] Screen initialization failed." << std::endl;
    }

    return true; 

}

// System shutdown
// Input: -
// Output: -
// Initialize speaker_test
// Read the speaker_test file into test_scripts
void RobotSystem::Init_speaker_test(){

    std::ifstream test_file(Config::Test::SPEAKER_TEST);
    std::string line;

    if (test_file.is_open()) {
        while (std::getline(test_file, line)) {
            if (!line.empty()) { 
                test_scripts.push_back(line);
            }
        }
        test_file.close();
    } else {
        std::cerr << "[Error][CogniArm]can't open test document! (/test/speaker_test.txt)" << std::endl;
    }
}
