#pragma once

#include <memory>
#include <atomic>
#include <string>

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

    std::unique_ptr<ScreenProducer> screenIn;
    std::unique_ptr<VoiceProducer> voiceIn;

    std::unique_ptr<RobotBrain> brain;
    std::unique_ptr<TaskSupervisor> supervisor;

    std::shared_ptr<TaskMonitor> globalMonitor;

    std::atomic<bool> isRunning{false};

public:
    RobotSystem();
    ~RobotSystem(); 

    bool init();
    void start();
    void stop();

};