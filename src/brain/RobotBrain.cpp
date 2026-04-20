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

// Initialize Nlu
// Configure host_name;robot_name
RobotBrain::RobotBrain(std::shared_ptr<SpeakerExecutor> s, 
                       std::shared_ptr<MotionExecutor> m, 
                       std::shared_ptr<CameraExecutor> c,
                       std::shared_ptr<TaskMonitor> taskMonitor)
    : speaker(s), motion(m), camera(c), _taskMonitor(taskMonitor) {
    
    // Initialize the NLU engine
    nlu = std::make_unique<NLUEngine>(Config::Nlu::NLU_MODEL_DIR); 
    if(nlu) {
        nlu->init();
        std::cout << "[Init][RobotBrain] Logic engine is ready" << std::endl;
    }

    config_var _var;
    _var = screen_get_var();
    _currentLang = _var.lang;
    _host_name = _var.host;
    _robot_name = _var.robot;
}

RobotBrain::~RobotBrain() = default;

// Callback function
// Process tasks received from the microphone and forward them to the executor queue
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

    if( text == "update"){
        if(camera) camera->pushTask("UPDATEBG");
        return;
    }
    if ( text == "reset"){
        if(motion) motion->pushTask("RESET");
        if(speaker) speaker->pushTask(speaker->getText("reset"));
        return;
    }

    auto res = nlu->predict(text);
    if(!speaker){
        std::cout << "[RobotBrain][Microphone_Test]raw_text:" << text << std::endl;
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
            std::cout << "[Info][RobotBrain] NLU failed to recognize a valid intent:" << text << std::endl;
            if (speaker) speaker->pushTask(speaker->getText("what_do_you_say"));
        }

    }else{
        // Successfully recognized the correct value
        if(!speaker){
        std::cout << "[RobotBrain][Nlu_Test]" << "nlu_intent: " << res.intent << "nlu_value: " << res.currentValue << std::endl;
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

// Callback function
// Process tasks received from the screen and forward them to the executor queue
void RobotBrain::handleUISignal(const std::string& type, const std::string& data) {
    
    if (type == "STOP_SYSTEM") {
        std::cout << "[End][RobotBrain] Received the system exit signal" << std::endl;
        _exit_signal = true;
        return;
    }

    if (isLearningMode){
       
        if( (type == "MOTION_CONFIRM") || (type == "DO_MOTION") ){
            std::cout << "[RobotBrain][onLearningMode]" << type << std::endl;
            btn_detected(type, data);
            if(!motion) return;
            return;
        }else{
            std::cout << "[Error][RobotBrain]Invalid signal" << std::endl;
            return;
        }

    }else if (!btn_detected(type, data)) {
        std::cout << "[Info][RobotBrain] Failed to recognize a valid UI signal: " << type << std::endl;
        return;
    }
}


// --------------------Intent detection--------------
// Parse the NLU output and dispatch different tasks based on the intent type 
// Input: nlu_output
// Output: false -- failed to detect a valid intent
//         true -- successfully detected the intent and dispatched the task
bool RobotBrain::nlu_detected(const nlu_output& res) {
    IntentType type = parseIntent(res.intent);

    switch (type) {
        case IntentType::OTHER:
        case IntentType::UNKNOWN:
            std::cout << "[Info][RobotBrain] NLU failed to recognize a valid intent" << std::endl;
            return false;

        case IntentType::DO_MOTION:
        case IntentType::FIND_OBJ:
        case IntentType::LEARN_MOTION:
        case IntentType::LEARN_OBJ:
            if (res.currentValue.empty()) {
                std::cout << "[Info][RobotBrain] NLU recognized the intent but required parameters are missing" << std::endl;
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

// --------------------Intent extraction------------------------
// A simple manual implementation for intent extraction.More complex rules or models can be introduced in the future.
// Handles tasks when NLU parsing fails by extracting keywords.
// This logic should be updated if multilingual support is introduced later.
// Input: rawText
// Output: false -- no keyword matched
//         true -- matched successfully
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
        // please do motion dance
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

// btn intent dispatch
// Input: type -- signal type
//        data -- signal data
// Output: false -- failed to recognize the signal type
//         true -- recognized successfully
bool RobotBrain::btn_detected(const std::string& type, const std::string& data) {

    
    if (type == "MOTION_LEARN") {
        isLearningMode = true;
        if(motion) motion->pushTask("LEARNMOTION:" + data);
        if(speaker) speaker->pushTask(speaker->getText("learn_moion_now") + data);
        _lastlearnmotion = data; // Record the motion currently being learned
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

// Update the supervisor state based on the motion learning result
void RobotBrain::SetState(bool state){
    if(state){
        isLearningMode = true;
    }else{
        isLearningMode = false;
        motion->end_learnning_mode();
    }
}
