#pragma once

#include "Executor.hpp"
#include "BugCode.hpp"
#include "Config.hpp"
#include "TaskMonitor.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <vector>

class MotionManager;

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

enum class LearningCode{
    Success = 0,
    LearningInit = 1,  // 初始化状态（等待学习指令
    LearningProcess = 2,  //学习进行中
    LearninginputError = 3, // 噪声太多退出
    LearningQuit = 4, // 后续追加
    LearningSaveError = 5, // motionset文档存储错误
    LearningQueueError = 6,
};

class MotionExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<MotionManager> manager;
    std::shared_ptr<TaskMonitor> _taskMonitor;

    std::atomic<ARMMODE> armMode = ARMMODE::IDLE;

public:
    MotionExecutor(std::shared_ptr<TaskMonitor> taskMonitor);
    ~MotionExecutor(); 
    
    void pinThread(int num);

    // 处理来自 Brain 的异步动作指令
    void onExecute(const std::string& motionName) override;

    void _stop() override;

    void _start(int core) override;

    std::string get_module_name() override;

    // 测试用
    void doDirectTask(const MotionTask& t);

    // 学习状态
    bool checklearningstate();

    // 意外退出检查
    bool check_acclearning_stop();

    void get_obj_APP(int position_x,int position_y);



private:    
    MotionCommand analyzecommand(const std::string& task);
    void HandleState(BugCode_M msg);

    std::atomic<bool> _islearningfinish = false;

    std::string motion_name = "None";

    LearningCode learning_state = LearningCode::LearningInit;

    std::atomic<int> task_id = 0;
    
    EmptyResult bg;

};