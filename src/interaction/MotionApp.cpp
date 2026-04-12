#include "MotionApp.hpp"

#include "MotionManager.hpp"
#include "MotionHandle.hpp"
#include "config_voice.hpp" 

#include <nlohmann/json.hpp>
#include <iostream>



// 需要考虑一个问题，这个模块到底管理哪一部分任务？
// 任务层是不是需要负责任务的学习比较好？
// brain只负责改变状态，如果是学习状态则直接把所有信息传给motion，motion来负责学习？

MotionExecutor::MotionExecutor() {

    manager = std::make_unique<MotionManager>(Config::Motion::MOTION_CONFIG);
    std::cout << "[MotionExecutor] 动作管理器已初始化" << std::endl;

    
}

MotionExecutor::~MotionExecutor() = default; 

void MotionExecutor::pinThread(int num){
    pinThreadToCore(this->worker,"MotionTask",num);
}

void MotionExecutor::onExecute(const std::string& task) {

    if (!manager) return;

    // 这里执行motion相关的内容
    // 收到的指令如下
    // 这个模块只负责指导机器人动作！因此learnmotion，confirm都不需要在这里
    // 只需要知道，如果是单一motion，就移动，motionset，就执行整个set，reset，就重置，stop，就停！
    // 你说的不对，这一个模块应该也负责learning和confirm，大脑只负责将task分到motion，speaker，camera三个模块
    // DOMOTION:
    // MOTIONSET:
    // RESET
    // STOP
    // LEARNMOTION
    // CONFIRM

    // 小动作为
    // left/right + u/d/r/l

    // Motion还是一样，直接简单分词

    // 每次执行还是先清空下，以免上一个动作的obj没删

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
            std::cout << "[Success] Operation completed successfully\n";
            break;
        case BugCode_M::LearningSuccess:
            std::cout << "[LearningSuccess] Learning completed successfully\n";
            break;
        case BugCode_M::NoMotion:
            std::cout << "[NoMotion] No motion available\n";
            break;
        case BugCode_M::CannotOpenMotionFile:
            std::cout << "[CannotOpenMotionFile] Failed to open motion file\n";
            break;
        case BugCode_M::ReadInvalidSet:
            std::cout << "[ReadInvalidSet] Invalid motion set when reading\n";
            break;
        case BugCode_M::MotionSaveWrong:
            std::cout << "[MotionSaveWrong] Motion save failed\n";
            break;
        case BugCode_M::WriteInvalidSet:
            std::cout << "[WriteInvalidSet] Invalid motion set when writing\n";
            break;
        case BugCode_M::UnkonwJoint:
            std::cout << "[UnkonwJoint] Unknown joint type\n";
            break;
        case BugCode_M::MotionQueError:
            std::cout << "[MotionQueError] Motion queue error\n";
            break;
        case BugCode_M::Init:
            std::cout << "[Init] ?\n";
            break;
        default:
            std::cout << "[Unknown] Unknown BugCode_M\n";
            break;
    }
}
