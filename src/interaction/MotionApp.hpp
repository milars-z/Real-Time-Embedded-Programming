#pragma once

#include "Executor.hpp"
#include <string>
#include <memory>

class MotionManager;
struct MotionTask; 

class MotionExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<MotionManager> manager;

public:
    MotionExecutor();
    ~MotionExecutor(); 

    // 处理来自 Brain 的异步动作指令
    void onExecute(const std::string& motionName) override;

    // 测试用
    void doDirectTask(const MotionTask& t);
};