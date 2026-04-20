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
public:
    MotionExecutor(std::atomic<int>& system_stete, std::shared_ptr<TaskMonitor> taskMonitor);
    ~MotionExecutor(); 
    
    void pinThread(int num);

    void _stop() override;

    void _start(int core) override;

    // Brain 调用，结束学习模式
    void end_learnning_mode();

    // 从supervisor处获取检测到的obj的位置坐标
    void get_obj_APP(int position_x,int position_y);

private:

    // 主要执行函数
    void onExecute(const std::string& motionName) override;

    // 执行者内部函数，获取当前模块名称
    std::string get_module_name() override;

    // 学习状态
    bool checklearningstate();

    // 意外退出检查
    bool check_acclearning_stop();

    // 解析来自brain的指令
    MotionCommand analyzecommand(const std::string& task);

    // 解析模块状态
    void HandleState(BugCode_M msg);


private:    

    std::unique_ptr<MotionManager> manager;
    std::shared_ptr<TaskMonitor> _taskMonitor;

    std::atomic<ARMMODE> armMode = ARMMODE::IDLE;

    std::atomic<bool> _islearningfinish = false;

    std::string motion_name = "None";

    std::atomic<int> task_id = 0;
    
    EmptyResult bg;

};