#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>
#include <filesystem>

#include "SpeakerApp.hpp"

#include "TaskMessage.hpp"
#include "TaskMonitor.hpp"
#include "Tools.hpp"
#include "Config.hpp"

class SpeakerExecutor;
class MotionExecutor;

struct SystemConfig;

class TaskSupervisor {
public:

    TaskSupervisor( std::shared_ptr<TaskMonitor> monitor,
                    std::shared_ptr<MotionExecutor> motion,
                    std::shared_ptr<SpeakerExecutor> speaker,
                    SystemConfig sys_cfg); 
    ~TaskSupervisor(); 

    void start_thread(int core);

    void stop_thread();

private:

    void Initfile();

    void processLoop();

    void handleTaskResult(const TaskEvent& e);

    void MotionT(int x, int y);

    void SpeakerT(std::string Command);

    
    std::thread _TaskWorker;
    std::atomic<bool> _running;

    std::shared_ptr<TaskMonitor> _monitor;
    std::shared_ptr<SpeakerExecutor> _speaker;
    std::shared_ptr<MotionExecutor> _motion;

    // task_map : use to store time
    std::unordered_map<int, std::chrono::steady_clock::time_point> _pendingTasks;

    std::ofstream _logFile;

    SystemConfig _sys_cfg;
};