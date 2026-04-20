#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "Config.hpp" 

#include <nlohmann/json.hpp>
#include <iostream>

/**
 * @brief Constructor for MotionExecutor, initializes motion manager
 * @param system_stete Reference to system state atomic variable
 * @param taskMonitor Shared pointer to task monitor
 */
MotionExecutor::MotionExecutor(std::atomic<int>& system_stete, std::shared_ptr<TaskMonitor> taskMonitor) 
:_taskMonitor(taskMonitor)
{

    manager = std::make_unique<MotionManager>(system_stete, Config::Motion::MOTION_CONFIG, Config::Camera::CAMERA_CONFIG, _taskMonitor);
    std::cout << "[Init][MotionApp] MotionManager initialized" << std::endl;

}

/**
 * @brief Destructor for MotionExecutor, resets motion manager
 */
MotionExecutor::~MotionExecutor() {

    manager->excuteReset();

    std::cout << "[End][MotionApp] destructor end" << std::endl;
}

/**
 * @brief Get the module name
 * @return Module name as string
 */
std::string MotionExecutor::get_module_name(){
    return "Motion";
}

/**
 * @brief Stop the internal worker thread
 */
void MotionExecutor::_stop(){
    if(manager){
        manager->stop_thread();
        std::cout << "[End][MotionApp] Worker thread exited..." << std::endl;
    }
}

/**
 * @brief Start the internal worker thread
 * @param core CPU core number to pin the thread to
 */
void MotionExecutor::_start(int core){
    if(!manager) return;
    manager->start_thread(core);
    std::cout << "[Init][MotionApp] Internal thread started, pinned to core:" << core << std::endl;
}

/**
 * @brief Pin the worker thread to a specific CPU core
 * @param num CPU core number to pin to
 */
void MotionExecutor::pinThread(int num){
    pinThreadToCore(this->worker,"MotionTask",num);
}

/**
 * @brief Execute motion task based on command string
 * @param task Task command string to execute
 */
void MotionExecutor::onExecute(const std::string& task) {

    if (!manager) return;


    if(armMode == ARMMODE::IDLE){
        
        MotionCommand cmd;
        BugCode_M state;

        std::cout << "[Info][MotionApp] Executing task: " << task << std::endl;
        cmd = analyzecommand(task);
        // do simple motion
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
        // 重置
        else if(cmd.command == "RESET"){
            manager->excuteReset();
        }
        // 暂停
        else if(cmd.command == "STOP"){
            manager->excuteStop();
        }
        // 开启学习模式
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



/**
 * @brief Analyze and parse motion command from text
 * @param text Command text to analyze
 * @return Parsed MotionCommand structure
 */
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

/**
 * @brief Process detected object on motion app side
 * @param position_x X position of detected object
 * @param position_y Y position of detected object
 */
void MotionExecutor::get_obj_APP(int position_x,int position_y){

    std::cout << "[Info][MotionApp]detect : " << position_x  << "......" << position_y << std::endl;
    manager->excuteReset();
    manager->get_obj_MANA(position_x,position_y);
    
}

/**
 * @brief End learning mode based on supervisor feedback
 */
void MotionExecutor::end_learnning_mode(){
    armMode = ARMMODE::IDLE;
}
