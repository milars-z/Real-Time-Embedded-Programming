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
    std::shared_ptr<TaskMonitor> _taskMonitor;

    std::atomic<ARMMODE> armMode = ARMMODE::IDLE;

public:
    MotionExecutor(std::atomic<int>& system_stete, std::shared_ptr<TaskMonitor> taskMonitor);
    ~MotionExecutor(); 
    
    void pinThread(int num);

    void onExecute(const std::string& motionName) override;

    void _stop() override;

    void _start(int core) override;

    std::string get_module_name() override;

    // 学习状态
    bool checklearningstate();

    // 意外退出检查
    bool check_acclearning_stop();

    void get_obj_APP(int position_x,int position_y);

    // Brain 调用，结束学习模式
    void end_learnning_mode();

private:    
    MotionCommand analyzecommand(const std::string& task);
    void HandleState(BugCode_M msg);

    std::atomic<bool> _islearningfinish = false;

    std::string motion_name = "None";

    std::atomic<int> task_id = 0;
    
    EmptyResult bg;

};