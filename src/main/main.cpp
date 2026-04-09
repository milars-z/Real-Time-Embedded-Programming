// #include "voiceInteraction.hpp"
#include "cogniArm.hpp"
// #include "MotionManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// #include "Screen_ui.hpp"
#include <unistd.h>
#include <iostream>

#include <csignal>
#include <atomic>

std::atomic<bool> _exit_signal = false;

void handleSigint(int) {
    _exit_signal = true;
}


int main() {
    
    // 退出信号处理
    std::signal(SIGINT, handleSigint);
    RobotSystem robot;

    if (!robot.init()) {
        return -1;
    }

    robot.start(); 

    return 0;
}