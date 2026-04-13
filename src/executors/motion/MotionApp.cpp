#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "Config.hpp" 

#include <nlohmann/json.hpp>
#include <iostream>



// 需要考虑一个问题，这个模块到底管理哪一部分任务？
// 任务层是不是需要负责任务的学习比较好？
// brain只负责改变状态，如果是学习状态则直接把所有信息传给motion，motion来负责学习？

MotionExecutor::MotionExecutor() {

    manager = std::make_unique<MotionManager>(Config::Motion::MOTION_CONFIG);
    std::cout << "[MotionApp] MotionManager initialized" << std::endl;

}

MotionExecutor::~MotionExecutor() {
    std::cout << "[MotionApp] destructor end" << std::endl;
}

std::string MotionExecutor::get_module_name(){
    return "Motion";
}

// 阻塞退出
void MotionExecutor::_stop(){
    if(manager){
        manager->stop_thread();
        std::cout << "[MotionApp] Worker thread exited..." << std::endl;
    }
}

void MotionExecutor::_start(int core){
    manager->start_thread(core);
    std::cout << "[MotionApp] Internal thread started, pinned to core:" << core << std::endl;
}


void MotionExecutor::pinThread(int num){
    pinThreadToCore(this->worker,"MotionTask",num);
}

void MotionExecutor::onExecute(const std::string& task) {

    if (!manager) return;

    // 非学习模式
    if(armMode == ARMMODE::IDLE){
        
        MotionCommand cmd;
        BugCode_M state;

        std::cout << "[Motion] 正在执行动作: " << task << std::endl;
        cmd = analyzecommand(task);
        // 简易动作
        if (cmd.command == "DO_MOTION"){
            manager->excuteTask(cmd.obj);
        }
        // motion set
        else if(cmd.command == "MOTIONSET"){
            state = manager->excuteMotionSet(cmd.obj);
            HandleState(state);
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
            _islearningfinish = false;
            motion_name = cmd.obj;
        }
        // 确定，一般不会走到这里
        else if(cmd.command == "CONFIRM"){
            // armMode == ARMMODE::IDLE;
        }
    }
    // 学习模式，在这个模式下只会接收简易动作
    // 做的事情有两件，让下级执行动作与记录motionset
    else if(armMode == ARMMODE::LEARNING){
        
        // 开启学习模式后是什么逻辑？
        // brain收到的信息分发改变了，不需要给speaker？
        // brain会收到来自screen和mic两个板块的信息，在这里要做一些变更，但是理解层面应该是让brain做
        // 好的在brain侧处理下信息
        // brain不负责处理信息，直接全部送过来
        BugCode_M state;
        _islearningfinish = false;
        state = manager->processLearningInput(task,motion_name);
        if (state == BugCode_M::LearningSuccess){
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            motion_name = "None";
        }else if(state != BugCode_M::Success ){
            HandleState(state);
        }
    }
    


}

// 学习阶段的时候用，进来一个格式化text，经过小分词器直接执行任务
void MotionExecutor::doDirectTask(const MotionTask& t) {
    if (!manager) return;
}

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

bool MotionExecutor::checklearningstate(){
    return _islearningfinish;
}

void MotionExecutor::HandleState(BugCode_M code){
    switch (code) {
        case BugCode_M::Success:
            std::cout << "[Success][MotionApp] Operation completed successfully\n";
            break;
        case BugCode_M::LearningSuccess:
            std::cout << "[LearningSuccess][MotionApp] Learning completed successfully\n";
            break;
        case BugCode_M::NoMotion:
            std::cout << "[NoMotion][MotionApp] No motion available\n";
            break;
        case BugCode_M::CannotOpenMotionFile:
            std::cout << "[CannotOpenMotionFile][MotionApp] Failed to open motion file\n";
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            break;
        case BugCode_M::ReadInvalidSet:
            std::cout << "[ReadInvalidSet][MotionApp] Invalid motion set when reading\n";
            break;
        case BugCode_M::MotionSaveWrong:
            std::cout << "[MotionSaveWrong][MotionApp] Motion save failed\n";
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            break;
        case BugCode_M::WriteInvalidSet:
            std::cout << "[WriteInvalidSet][MotionApp] Invalid motion set when writing\n";
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            break;
        case BugCode_M::UnkonwJoint:
            std::cout << "[UnkonwJoint][MotionApp] Unknown joint type\n";
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            break;
        case BugCode_M::MotionQueError:
            std::cout << "[MotionQueError][MotionApp] Motion queue error\n";
            break;
        case BugCode_M::Init:
            std::cout << "[Init][MotionApp] ?\n";
            _islearningfinish = true;
            armMode = ARMMODE::IDLE;
            break;
        default:
            std::cout << "[Unknown][MotionApp] Unknown BugCode_M\n";
            break;
    }
}
