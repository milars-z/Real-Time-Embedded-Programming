#include "voiceInteraction.hpp"
// #include "MotionManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    RobotCore robot_interactive;
    // MotionManager *RobotMotion = nullptr;
    // RobotMotion = new MotionManager("../servo_config.txt");

    if (!robot_interactive.init()) {
        std::cerr << "Initialization failed." << std::endl;
        return -1;
    }

    robot_interactive.start();

    while (robot_interactive.running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }

    robot_interactive.stop();

    return 0;
}