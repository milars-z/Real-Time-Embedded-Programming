#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "MotionHandle.hpp"
#include "config_voice.hpp" 
#include <iostream>

MotionExecutor::MotionExecutor() {

    manager = std::make_unique<MotionManager>(Config::Motion::MOTION_CONFIG);
    std::cout << "[MotionExecutor] 动作管理器已初始化" << std::endl;
}

MotionExecutor::~MotionExecutor() = default; 

void MotionExecutor::onExecute(const std::string& motionName) {
    if (!manager) return;

    std::cout << "[Motion] 正在执行动作: " << motionName << std::endl;

    if (motionName == "INIT") {
        manager->servo_set_init();
    } else {
        manager->read_motion_set(motionName);
    }
}

void MotionExecutor::doDirectTask(const MotionTask& t) {
    if (manager) {
        manager->enqueue_motion(t);
    }
}