#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <vector>

#include "NluHandle.hpp"


class SpeakerExecutor;
class MotionExecutor;
class CameraExecutor;
class NLUEngine;


enum class IntentType {
    OTHER,
    DO_MOTION,
    FIND_OBJ,
    LEARN_MOTION,
    LEARN_OBJ,
    CHECK_HOST_NAME,
    CHECK_ROT_NAME,
    GREET,
    BYE,
    UNKNOWN
};

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

    bool nlu_detected(const nlu_output& res);

    bool btn_detected(const std::string& type, const std::string& data);

    bool extractIntent(const std::string& text);

    IntentType parseIntent(const std::string& intent);

    std::vector<std::string> split_text(const std::string& text);

    // 大脑记一下上一个学的是什么动作不过分吧
    std::string _lastlearnmotion = "None" ;

    std::string _currentJoint = "left_";

    std::string _currentWay = "l";
};