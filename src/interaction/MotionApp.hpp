#pragma once

#include "Executor.hpp"
#include "BugCode.hpp"
#include "config_voice.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <vector>

class MotionManager;

// 这是底层模块的Task，
// struct MotionTask {

//     Joint joint;   // "Base" / "Shoulder" / "Elbow"
//     MoveMethod method;
//     float targetAngle = 0.0;       
//     int motionSpeed = 100;     

// };
struct MotionTask; 

// 和camera保持一致吧
struct MotionCommand{
    std::string command;
    std::string obj;
};

enum class ARMMODE {
    IDLE,
    LEARNING,
    EXPLORE,
};

class MotionExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<MotionManager> manager;
    // 动作的学习代码写在APP层
    std::atomic<ARMMODE> armMode = ARMMODE::IDLE;

public:
    MotionExecutor();
    ~MotionExecutor(); 

    // 处理来自 Brain 的异步动作指令
    void onExecute(const std::string& motionName) override;

    // 测试用
    void doDirectTask(const MotionTask& t);


private:    
    MotionCommand analyzecommand(const std::string& task);
    void HandleState(BugCode_M msg);


    // 屎山搬家来了
    BugCode_M processLearningInput(const std::string& text);
    BugCode_M saveMotionSet(std::string motionName, std::vector<MotionTask>& rawTasks);

    std::string _currentLearningName = "";
    std::vector<MotionTask> _tempTasks ; 
    Joint _currentJoint = Joint::Base;
    std::string motionsetPath = Config::Motion::MOTION_SET;
};