// #include "voiceInteraction.hpp"
#include "cogniArm.hpp"
#include "system_config.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#include <unistd.h>
#include <iostream>

#include <csignal>
#include <atomic>

std::atomic<bool> _exit_signal = false;

void handleSigint(int) {
    _exit_signal = true;
}


int main(int argc, char* argv[]) {
    
    // 退出信号处理
    std::signal(SIGINT, handleSigint);
    {
        RobotSystem robot;

        std::string mode = "normal";

        SystemConfig cfg;

        for (int i = 1; i < argc; ++i){
            std::string arg = argv[i]; 
            if (arg == "--test" && i + 1 < argc){
                mode = argv[++i]; 
            }
        }

        cfg = makecfg(mode);
                
        if (!robot.init(cfg)) {
            return -1;
        }

        robot.start(); 
    }
    std::cout << "[main] system close successfully" << std::endl;
    return 0;
}