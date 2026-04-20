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

/**
 * @brief Motion execution module for the robotic arm.
 *
 * MotionExecutor receives commands from the brain module,
 * parses motion instructions, and delegates execution to MotionManager.
 * It also handles learning mode and object-based motion control.
 */
class MotionExecutor : public BaseExecutor<std::string> {
public:
    MotionExecutor(std::atomic<int>& system_stete, std::shared_ptr<TaskMonitor> taskMonitor);
    ~MotionExecutor(); 
    
    void pinThread(int num);

    void _stop() override;

    void _start(int core) override;

    // Called by Brain to exit learning mode
    void end_learnning_mode();

    // Receive detected object position from supervisor
    void get_obj_APP(int position_x,int position_y);

private:

    // Main execution function
    void onExecute(const std::string& motionName) override;

    // Internal executor function to retrieve the current module name
    std::string get_module_name() override;

    // Check learning state
    bool checklearningstate();

    // Check for unexpected learning interruption
    bool check_acclearning_stop();

    // Parse commands from Brain
    MotionCommand analyzecommand(const std::string& task);

    // Process module state
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
