#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "Config.hpp" 

#include <nlohmann/json.hpp>
#include <iostream>


MotionExecutor::MotionExecutor(std::atomic<int>& system_stete, std::shared_ptr<TaskMonitor> taskMonitor) 
:_taskMonitor(taskMonitor)
{

    manager = std::make_unique<MotionManager>(system_stete, Config::Motion::MOTION_CONFIG, Config::Camera::CAMERA_CONFIG, _taskMonitor);
    std::cout << "[Init][MotionApp] MotionManager initialized" << std::endl;

}

MotionExecutor::~MotionExecutor() {

    manager->excuteReset();

    std::cout << "[End][MotionApp] destructor end" << std::endl;
}

std::string MotionExecutor::get_module_name(){
    return "Motion";
}

// Blocking shutdown
void MotionExecutor::_stop(){
    if(manager){
        manager->stop_thread();
        std::cout << "[End][MotionApp] Worker thread exited..." << std::endl;
    }
}

void MotionExecutor::_start(int core){
    if(!manager) return;
    manager->start_thread(core);
    std::cout << "[Init][MotionApp] Internal thread started, pinned to core:" << core << std::endl;
}


void MotionExecutor::pinThread(int num){
    pinThreadToCore(this->worker,"MotionTask",num);
}

void MotionExecutor::onExecute(const std::string& task) {

    if (!manager) return;

    // Non-learning mode
    if(armMode == ARMMODE::IDLE){
        
        MotionCommand cmd;
        BugCode_M state;

        std::cout << "[Info][MotionApp] Executing motion: " << task << std::endl;
        cmd = analyzecommand(task);
        // Simple motion
        if (cmd.command == "DO_MOTION"){
            manager->excuteTask(cmd.obj);

#ifdef TESTMODE
                TaskEvent _taskevent;
                TaskDescribe _taskdescribe;
                _taskevent.moduleName = "Screen-Motion";
                _taskevent.taskId = task_id++;
                _taskdescribe.Name = cmd.obj;
                _taskdescribe.TaskType = cmd.command;
                _taskevent.taskType = _taskdescribe;
                _taskevent.status = TaskStatus::FINISHED;
                _taskevent.result = bg;
                _taskevent.timestamp = std::chrono::steady_clock::now();
                _taskMonitor->postEvent(_taskevent);
#endif

        }
        // motion set
        else if(cmd.command == "MOTIONSET"){
            manager->excuteMotionSet(cmd.obj);
        }
        // Reset
        else if(cmd.command == "RESET"){
            manager->excuteReset();
        }
        // Pause
        else if(cmd.command == "STOP"){
            manager->excuteStop();
        }
        // Enter learning mode
        else if(cmd.command == "LEARNMOTION"){
            armMode = ARMMODE::LEARNING;
            motion_name = cmd.obj;
        }
        else if(cmd.command == "CONFIRM"){
            armMode == ARMMODE::IDLE;
        }
    }

    else if(armMode == ARMMODE::LEARNING){
        
        manager->processLearningInput(task,motion_name);

    }
}


// Parse string commands
// Input: text
// Output: MotionCommand
MotionCommand MotionExecutor::analyzecommand(const std::string& text){

    MotionCommand cmd;

    if (text == "RESET") {
        cmd.command = "RESET";
        cmd.obj = "";
        return cmd;
    }

    if (text == "STOP") {
        cmd.command = "STOP";
        cmd.obj = "";
        return cmd;
    }

    if (text == "CONFIRM") {
        cmd.command = "CONFIRM";
        cmd.obj = "";
        return cmd;
    }

    const std::string do_prefix = "DOMOTION:";
    if (text.rfind(do_prefix, 0) == 0) {
        cmd.command = "DO_MOTION";
        cmd.obj = text.substr(do_prefix.size());
        return cmd;
    }

    const std::string set_prefix = "MOTIONSET:";
    if (text.rfind(set_prefix, 0) == 0) {
        cmd.command = "MOTIONSET";
        cmd.obj = text.substr(set_prefix.size());
        return cmd;
    }

    const std::string learn_prefix = "LEARNMOTION:";
    if (text.rfind(learn_prefix, 0) == 0) {
        cmd.command = "LEARNMOTION";
        cmd.obj = text.substr(learn_prefix.size());
        return cmd;
    }

    return cmd;
}

// Handle detected objects on the MotionApp side
void MotionExecutor::get_obj_APP(int position_x,int position_y){

    std::cout << "[Info][MotionApp]detect : " << position_x  << "......" << position_y << std::endl;
    manager->excuteReset();
    manager->get_obj_MANA(position_x,position_y);
    
}

// Externally callable
// Exit learning mode based on feedback from the supervisor
void MotionExecutor::end_learnning_mode(){
    armMode = ARMMODE::IDLE;
}
