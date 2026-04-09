#pragma once

#include <memory>
#include <string>
#include <atomic>

class SpeakerExecutor;
class MotionExecutor;
class CameraExecutor;
class NLUEngine;


class RobotBrain {
private:
    std::shared_ptr<SpeakerExecutor> speaker;
    std::shared_ptr<MotionExecutor> motion;
    std::shared_ptr<CameraExecutor> camera;
    std::unique_ptr<NLUEngine> nlu;
    
    std::atomic<bool> isLearningMode = false;

public:
    
    RobotBrain(std::shared_ptr<SpeakerExecutor> s, 
               std::shared_ptr<MotionExecutor> m, 
               std::shared_ptr<CameraExecutor> c);
    

    ~RobotBrain();

    void handleIncomingText(const std::string& text);

    void handleUISignal(const std::string& type, const std::string& data);

private:
    // 内部状态，有点突兀后续找办法解决
    void processLearning(const std::string& text);
};