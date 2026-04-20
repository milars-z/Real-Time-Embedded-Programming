#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <vector>

#include "NluHandle.hpp"
#include "TaskMonitor.hpp" 
#include "Tools.hpp"

class SpeakerExecutor;
class MotionExecutor;
class CameraExecutor;
class NLUEngine;

/**
 * @brief Core logic module of the robot system.
 *
 * RobotBrain processes text input, UI signals, and NLU output,
 * and coordinates the speaker, motion, and camera executors.
 */
class RobotBrain {
private:
    std::shared_ptr<SpeakerExecutor> speaker;
    std::shared_ptr<MotionExecutor> motion;
    std::shared_ptr<CameraExecutor> camera;
    std::shared_ptr<TaskMonitor> _taskMonitor;
    std::unique_ptr<NLUEngine> nlu;
    
    std::atomic<bool> isLearningMode = false;

public:
    
    RobotBrain(std::shared_ptr<SpeakerExecutor> s, 
               std::shared_ptr<MotionExecutor> m, 
               std::shared_ptr<CameraExecutor> c,
               std::shared_ptr<TaskMonitor> taskMonitor);
    

    ~RobotBrain();

    void handleIncomingText(const std::string& text);

    void handleUISignal(const std::string& type, const std::string& data);

    // Called by the supervisor to update the state
    void SetState(bool state);

private:

    bool nlu_detected(const nlu_output& res);

    bool btn_detected(const std::string& type, const std::string& data);

    bool extractIntent(const std::string& text);

    std::string _lastlearnmotion = "None" ;

    std::string _currentJoint = "left_";

    std::string _currentWay = "l";

    std::atomic<int> task_id = 3000;

    TaskDescribe _taskdescribe;
    
    EmptyResult bg;

    std::string _currentLang = "en";
    std::string _host_name = "milars";
    std::string _robot_name = "robot";
};
