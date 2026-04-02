#include "voiceInteraction.hpp"
// #include "MotionManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    RobotCore robot_interactive;

    if (!robot_interactive.init()) {
        std::cerr << "Initialization failed." << std::endl;
        return -1;
    }

    robot_interactive.start();

    while (robot_interactive.running()) {

        cv::Mat view = robot_interactive.getCamHandle()->getProcessedFrame();

        if (!view.empty()) {
            cv::imshow("Robot Camera Demo", view);
        }

        // Cam测试
        int key = cv::waitKey(1); 
        if (key == 'q') break;
        
        if (key == 'b') robot_interactive.getCamHandle()->Update_bg();
        if (key == 's') robot_interactive.getCamHandle()->Learn_obj("obj_1");
        if (key == 'm') robot_interactive.getCamHandle()->Find_obj("obj_1");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }

    robot_interactive.stop();

    return 0;
}