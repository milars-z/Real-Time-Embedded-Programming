#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>

#include "system_config.hpp"

class SpeakerExecutor;
class MotionExecutor;
class CameraExecutor;
class ScreenProducer;
class VoiceProducer;
class RobotBrain;
class TaskMonitor;
class TaskSupervisor;

class RobotSystem {
private:

    std::shared_ptr<SpeakerExecutor> speaker;
    std::shared_ptr<MotionExecutor> motion;
    std::shared_ptr<CameraExecutor> camera;

    std::shared_ptr<RobotBrain> brain;

    std::unique_ptr<ScreenProducer> screenIn;
    std::unique_ptr<VoiceProducer> voiceIn;

    std::unique_ptr<TaskSupervisor> supervisor;
    std::shared_ptr<TaskMonitor> globalMonitor;

    std::atomic<bool> isRunning{false};

    // use for speaker test
    std::vector<std::string> test_scripts;
    int current_line_idx = 0;

    void Init_speaker_test();

    void start_thread();

    void stop();

    bool check_state(std::atomic<int>& state);

    SystemConfig sys_cfg;  

public:
    // system debug state
    std::atomic<int> state = 0;
    RobotSystem();
    ~RobotSystem(); 

    bool init(SystemConfig cfg);
    void start();


  
};