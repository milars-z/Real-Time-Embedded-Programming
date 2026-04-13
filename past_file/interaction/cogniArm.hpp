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

class RobotSystem {
private:

    std::shared_ptr<SpeakerExecutor> speaker;
    std::shared_ptr<MotionExecutor> motion;
    std::shared_ptr<CameraExecutor> camera;

    std::unique_ptr<ScreenProducer> screenIn;
    std::unique_ptr<VoiceProducer> voiceIn;

    std::unique_ptr<RobotBrain> brain;

    std::atomic<bool> isRunning{false};

public:
    RobotSystem();
    ~RobotSystem(); 

    bool init();
    void start();
    void stop();

};