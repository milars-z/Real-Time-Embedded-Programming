#include "RobotBrain.hpp"
#include "system_config.hpp"

#include "SpeakerApp.hpp"
#include "MotionApp.hpp"
#include "CameraApp.hpp"
#include "NluHandle.hpp"
#include "Config.hpp" 
#include <iostream>
#include <cstdio>

extern std::atomic<bool> _exit_signal; 
static const std::string _username = "milars";
static const std::string _robotname = "your robot";

// 初始化Nlu
// 配置host_name;robot_name
RobotBrain::RobotBrain(std::shared_ptr<SpeakerExecutor> s, 
                       std::shared_ptr<MotionExecutor> m, 
                       std::shared_ptr<CameraExecutor> c,
                       std::shared_ptr<TaskMonitor> taskMonitor)
    : speaker(s), motion(m), camera(c), _taskMonitor(taskMonitor) {
    
    // 初始化 NLU 引擎
    nlu = std::make_unique<NLUEngine>(Config::Nlu::NLU_MODEL_DIR); 
    if(nlu) {
        nlu->init();
        std::cout << "[Brain] 逻辑引擎已就绪" << std::endl;
    }

    config_var _var;
    _var = screen_get_var();
    _currentLang = _var.lang;
    _host_name = _var.host;
    _robot_name = _var.robot;
}

RobotBrain::~RobotBrain() = default;

// 回调函数
// 处理microphone所下发的任务，将任务下发给executor队列
void RobotBrain::handleIncomingText(const std::string& text) {

    if (text.empty()) return;

    TaskEvent _taskevent;
    _taskevent.moduleName = "Nlu";

    if(!speaker){
        isLearningMode = false;
    }
    
    if (isLearningMode) {
        if(!motion) return;
        motion->pushTask(text);
        return;
    }

#ifdef TESTMODE

    _taskdescribe.Name = text;
    _taskdescribe.TaskType = "nlu_analyez";
    _taskevent.status = TaskStatus::STARTED;
    _taskevent.taskId = task_id++;
    _taskevent.timestamp = std::chrono::steady_clock::now(); 
    _taskevent.taskType = _taskdescribe;
    _taskMonitor->postEvent(_taskevent);

    NluResult nluresult;

#endif 

    auto res = nlu->predict(text);
    if(!speaker){
        std::cout << "raw_text:" << text << std::endl;
    }

    if(!nlu_detected(res)){

#ifdef TESTMODE
        _taskevent.status = TaskStatus::FINISHED;
        nluresult.isdetecte = false;
        _taskevent.result = nluresult;
        _taskevent.issuccessful = false;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskMonitor->postEvent(_taskevent);
#endif
        if(!extractIntent(text)){
            std::cout << "[Brain] 未能识别有效意图:" << text << std::endl;
            if (speaker) speaker->pushTask(speaker->getText("what_do_you_say"));
        }

    }else{
        // 成功检测出正确的值
        if(!speaker){
        std::cout << "nlu_intent:" << res.intent << "nlu_value:" << res.currentValue << std::endl;
    }
#ifdef TESTMODE
        _taskevent.status = TaskStatus::FINISHED;
        nluresult.intent = res.intent;
        nluresult.name = res.currentValue;
        nluresult.isdetecte = true;
        _taskevent.result = nluresult;
        _taskevent.issuccessful = true;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskMonitor->postEvent(_taskevent);
#endif 

    }
}

// 回调函数
// 处理来自screen端的任务，将任务下发给executor队列
void RobotBrain::handleUISignal(const std::string& type, const std::string& data) {
    
    if (type == "STOP_SYSTEM") {
        std::cout << "[Brain] 收到系统退出信号" << std::endl;
        _exit_signal = true;
        return;
    }

    if (isLearningMode){
       
        if( (type == "MOTION_CONFIRM") || (type == "DO_MOTION") ){
            std::cout << "[onLearningMode]" << type << std::endl;
            btn_detected(type, data);
            if(!motion) return;
            return;
        }else{
            std::cout << "信号错误" << std::endl;
            return;
        }

    }else if (!btn_detected(type, data)) {
        std::cout << "[Brain] 未能识别有效 UI 信号: " << type << std::endl;
        return;
    }
}


// --------------------意图检测相关--------------
// 解析nlu处理后的信号，根据不同信号类型下发不同任务 
// 输入：rawText
// 输出：false -- 未能检测到准确意图
//       true -- 成功检测意图并下发任务
bool RobotBrain::nlu_detected(const nlu_output& res) {
    IntentType type = parseIntent(res.intent);

    switch (type) {
        case IntentType::OTHER:
        case IntentType::UNKNOWN:
            std::cout << "[Brain] NLU 未能识别有效意图" << std::endl;
            return false;

        case IntentType::DO_MOTION:
        case IntentType::FIND_OBJ:
        case IntentType::LEARN_MOTION:
        case IntentType::LEARN_OBJ:
            if (res.currentValue.empty()) {
                std::cout << "[Brain] NLU 识别到意图但缺少参数" << std::endl;
                return false;
            }
            break;

        default:
            break;
    }

    switch (type) {
        case IntentType::CHECK_HOST_NAME:
            if(speaker) speaker->pushTask(speaker->getText("check_host_name"));
            return true;
        case IntentType::CHECK_ROT_NAME:
            if(speaker) speaker->pushTask(speaker->getText("check_robot_name"));
            return true;
        case IntentType::GREET:
            if(speaker) speaker->pushTask(speaker->getText("welcome"));
            return true;
        case IntentType::DO_MOTION: {
            std::string motion_cmd = "MOTIONSET:" + res.currentValue;
            if(motion) motion->pushTask(motion_cmd);
            return true;
        }
        case IntentType::LEARN_MOTION: {
            std::string motion_cmd = "LEARNMOTION:" + res.currentValue;
            isLearningMode = true;
            if(motion) motion->pushTask(motion_cmd);
            if(speaker) speaker->pushTask(speaker->getText("learn_moion_now") + res.currentValue);
            return true;
        }
        case IntentType::LEARN_OBJ: {
            std::string obj_cmd = "LEARNOBJ:" + res.currentValue;
            if(camera) camera->pushTask(obj_cmd);
            return true;
        }
        case IntentType::FIND_OBJ: {
            std::string obj_cmd = "FINDOBJ:" + res.currentValue;
            if(camera) camera->pushTask(obj_cmd);
            return true;
        }
        case IntentType::BYE:
            if(speaker) speaker->pushTask(speaker->getText("bye"));
            return true;

        default:
            return false;
    }
}

// --------------------意图检测相关------------------------
// 手动提取意图的简单实现，后续可以用更复杂的规则或者模型来实现
// 处理nlu解析失败后的任务，提取关键词
// 后续引入多语言需要更新、
// 输入：rawText
// 输出：false -- 未匹配带关键词
//       true -- 成功匹配
bool RobotBrain::extractIntent(const std::string& text) {

    std::vector<std::string> tokens = split_text(text);
    
    for (size_t i = 0; i < tokens.size(); ++i) {

        // find xxx
        // find apple
        if (tokens[i] == "find" && i + 1 < tokens.size()) {
            if(camera) camera->pushTask("FINDOBJ:" + tokens[i + 1]);
            return true;
        }

        // do motion xxx
        // please do motio dance
        if (tokens[i] == "do" && i + 2 < tokens.size() && tokens[i + 1] == "motion") {
            if(motion) motion->pushTask("MOTIONSET:" + tokens[i + 2]);
            return true;
        }

        // learn motion xxx
        // please learn motion dance
        if (tokens[i] == "learn" && i + 2 < tokens.size() && tokens[i + 1] == "motion") {
            isLearningMode = true;
            _lastlearnmotion = tokens[i + 2];
            if(motion) motion->pushTask("LEARNMOTION:" + tokens[i + 2]);
            if(speaker) speaker->pushTask(speaker->getText("learn_moion_now") + tokens[i + 2]);
            return true;
        }

        // this is xxx
        // this is apple
        if (tokens[i] == "this" && i + 2 < tokens.size() && tokens[i + 1] == "is") {
            if(camera) camera->pushTask("LEARNOBJ:" + tokens[i + 2]);
            return true;
        }

        if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
            isLearningMode = false;
            if(speaker) speaker->pushTask(speaker->getText("learn_moion_finish") + _lastlearnmotion);
            _lastlearnmotion = "None";
            return true;
        }
    }

    return false; 

}

// btn意图分发
// 输入：type -- 信号类型
//      data -- 信号数据
// 输出：false -- 未能识别该信号类型
//      true -- 成功识别
bool RobotBrain::btn_detected(const std::string& type, const std::string& data) {
    // 传来的数据是固定的，因此不需要反复修改
    // MOTION_LEARN MOTION_CONFIRM  VISION_LEARN  VISION_DETECT  VISION_UPDATE DO_MOTION
    // 就写if了屏幕的后续扩展应该不多
    if (type == "MOTION_LEARN") {
        isLearningMode = true;
        if(motion) motion->pushTask("LEARNMOTION:" + data);
        if(speaker) speaker->pushTask(speaker->getText("learn_moion_now") + data);
        _lastlearnmotion = data; // 记录正在学习的动作
        return true;
    }else if (type == "MOTION_CONFIRM") {
        isLearningMode = false;
        if(motion) motion->pushTask("CONFIRM");
        _lastlearnmotion = "None";
        return true;
    }else if (type == "VISION_LEARN") {
        if(camera) camera->pushTask("LEARNOBJ:" + data);
        return true;
    }else if (type == "VISION_DETECT") {
        if(camera) camera->pushTask("FINDOBJ:" + data);
        return true;
    }else if (type == "VISION_UPDATE") {
        if(camera) camera->pushTask("UPDATEBG");
        return true;
    }else if (type == "DO_MOTION") {
        if(motion) motion->pushTask("DOMOTION:" + data);
        return true;
    }else if (type == "MOTION_SET") {
        if(motion) motion->pushTask("MOTIONSET:" + data);
        return true;
    }else if (type == "RESET") {
        if(motion) motion->pushTask("RESET");
        if(speaker) speaker->pushTask(speaker->getText("reset"));
        return true;
    }else if (type == "STOP") {
        if(motion) motion->pushTask("STOP");
        return true;
    }else if (type == "SET_HOST_NAME"){

        _host_name = data;

        saveVariablesToFile(_currentLang,_host_name,_robot_name);
        if(speaker) speaker->setVariable("host_name",_host_name);
        if(speaker) speaker->pushTask(speaker->getText("check_host_name"));

        return true;
        
    }else if (type == "SET_ROBOT_NAME"){

        _robot_name = data;

        saveVariablesToFile(_currentLang,_host_name,_robot_name);
        if(speaker) speaker->setVariable("robot_name",_robot_name);
        if(speaker) speaker->pushTask(speaker->getText("check_robot_name"));
        
        return true;
    }


    return false;
}

// supervisor侧根据motion学习的结果更改状态
void RobotBrain::SetState(bool state){
    if(state){
        isLearningMode = true;
    }else{
        isLearningMode = false;
    }
}
