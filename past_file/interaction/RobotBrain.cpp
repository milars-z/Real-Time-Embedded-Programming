// 该函数作为整体分发模块
// 所做的事情如下：
// 回调触发，接收来自screen和microphone的输入进行判断与任务分发
// 后续需要将此逻辑拆分成两个，一个用来接收回调，一个用来处理逻辑

#include "RobotBrain.hpp"

#include "SpeakerApp.hpp"
#include "MotionApp.hpp"
#include "CameraApp.hpp"
#include "NluHandle.hpp"
#include "config_voice.hpp" 
#include <iostream>
#include <cstdio>

extern std::atomic<bool> _exit_signal; // 退出
static const std::string _username = "milars";
static const std::string _robotname = "your robot";

// nlu返回的intent，处理成枚举类型，方便后续逻辑判断
IntentType RobotBrain::parseIntent(const std::string& intent) {
    if (intent == "other" || intent.empty()) return IntentType::OTHER;
    if (intent == "do_motion") return IntentType::DO_MOTION;
    if (intent == "find_obj") return IntentType::FIND_OBJ;
    if (intent == "learn_motion") return IntentType::LEARN_MOTION;
    if (intent == "learn_obj") return IntentType::LEARN_OBJ;
    if (intent == "check_host_name") return IntentType::CHECK_HOST_NAME;
    if (intent == "check_rot_name") return IntentType::CHECK_ROT_NAME;
    if (intent == "greet") return IntentType::GREET;
    if (intent == "bye") return IntentType::BYE;
    return IntentType::UNKNOWN;
}

RobotBrain::RobotBrain(std::shared_ptr<SpeakerExecutor> s, 
                       std::shared_ptr<MotionExecutor> m, 
                       std::shared_ptr<CameraExecutor> c)
    : speaker(s), motion(m), camera(c) {
    
    // 初始化 NLU 引擎
    nlu = std::make_unique<NLUEngine>(Config::Path::NLU_MODEL_DIR); 
    nlu->init();
    
    std::cout << "[Brain] 逻辑引擎已就绪" << std::endl;
}

RobotBrain::~RobotBrain() = default;



    // Screen_
    // std::string type
    // std:: string data
    // type: MOTION_LEARN
    // MOTION_CONFIRM
    // VISION_LEARN
    // VISION_DETECT
    // VISION_UPDATE
    // DO_MOTION

void RobotBrain::handleIncomingText(const std::string& text) {
    // 处理来自麦克风的文本信号
    if (text.empty()) return;
    
    // if(motion->checklearningstate()){
    //     isLearningMode = false;
    // }

    if (isLearningMode) {
        processLearning(text);

        return;
    }

    // 调用 NLU 解析意图
    // NLU会不会卡住？后面NLU大了之后得单独开个线程，先这样用再说
    auto res = nlu->predict(text);
    // std::cout << "[Brain] 意图识别: " << res.intent << " 参数: " << res.currentValue << std::endl;
    if(!nlu_detected(res)){
        // 意图没有正常处理就用分词去处理整体的text
        if(!extractIntent(text)){
            std::cout << "[Brain] 未能识别有效意图:" << text << std::endl;
            speaker->pushTask("sorry, i didn't understand that");
        }
    }
}

void RobotBrain::handleUISignal(const std::string& type, const std::string& data) {
    // 处理来自 ScreenApp 的 UI 交互信号
    
    // if(motion->checklearningstate()){
    //     isLearningMode = false;
    // }
    if (type == "STOP_SYSTEM") {
        std::cout << "[Brain] 收到系统退出信号" << std::endl;
        _exit_signal = true;
        return;
    }

    // std::cout << "[handleUISignal]" << type << std::endl;
    // std::cout << isLearningMode << std::endl;
    // 学习模式只处理两种信号
    if (isLearningMode){
       
        if( (type == "MOTION_CONFIRM") || (type == "DO_MOTION") ){
            std::cout << "[onLearningMode]" << type << std::endl;
            btn_detected(type, data);
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


// 学习模式下，需要对语音信号重新分词
// 分词以前有写在app里直接搬家
// 不搬家了，直接都丢进去
void RobotBrain::processLearning(const std::string& text) {
    // printf("[Brain] 学习模式 - 正在记录特征: %s\n", text.c_str());
    // 这里是学习模式的一些信息处理
    // 学习模式启动后在此进行逻辑分发

    // 后续应该再加个错误管理

    // 双重保险
    if(!isLearningMode) return;

    motion->pushTask(text);


}


// --------------------意图检测相关---------------------    

// 通过nlu模型进行识别产生结果
// 后续会移动该模块到判断模块中，所以分开写
bool RobotBrain::nlu_detected(const nlu_output& res) {
    IntentType type = parseIntent(res.intent);

    // 参数校验
    switch (type) {
        // 后面可能加other吧
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

    // 主逻辑处理,用case好像也没多简单，后续更新想尝试一下映射，后面还能改键位
    switch (type) {
        case IntentType::CHECK_HOST_NAME:
            speaker->pushTask("hello , you are " + _username);
            return true;
        case IntentType::CHECK_ROT_NAME:
            speaker->pushTask("hello, i am " + _robotname);
            return true;
        case IntentType::GREET:
            speaker->pushTask("hello good morning " + _username);
            return true;
        case IntentType::DO_MOTION: {
            std::string motion_cmd = "DOMOTION:" + res.currentValue;
            motion->pushTask(motion_cmd);
            speaker->pushTask("do motion " + res.currentValue);
            return true;
        }
        case IntentType::LEARN_MOTION: {
            std::string motion_cmd = "LEARNMOTION:" + res.currentValue;
            isLearningMode = true;
            motion->pushTask(motion_cmd);
            speaker->pushTask("learn motion " + res.currentValue);
            return true;
        }
        case IntentType::LEARN_OBJ: {
            std::string obj_cmd = "LEARNOBJ:" + res.currentValue;
            camera->pushTask(obj_cmd);
            speaker->pushTask("learn object " + res.currentValue);
            return true;
        }
        case IntentType::FIND_OBJ: {
            std::string obj_cmd = "FINDOBJ:" + res.currentValue;
            camera->pushTask(obj_cmd);
            speaker->pushTask("find object " + res.currentValue);
            return true;
        }
        case IntentType::BYE:
            speaker->pushTask("good bye " + _username);
            return true;

        default:
            return false;
    }
}

// 配合关键词提取用
std::vector<std::string> RobotBrain::split_text(const std::string& text) {
    std::stringstream ss(text);
    std::string word;
    std::vector<std::string> tokens;

    while (ss >> word) {
        tokens.push_back(word);
    }
    return tokens;
}

// 关键词提取的简单实现，后续可以用更复杂的规则或者模型来实现
bool RobotBrain::extractIntent(const std::string& text) {
    std::vector<std::string> tokens = split_text(text);
    // 手动提取意图的简单实现，后续可以用更复杂的规则或者模型来实现
    for (size_t i = 0; i < tokens.size(); ++i) {

        // find xxx
        // find apple
        if (tokens[i] == "find" && i + 1 < tokens.size()) {
            camera->pushTask("FINDOBJ:" + tokens[i + 1]);
            speaker->pushTask("find " + tokens[i + 1]);
            return true;
        }

        // do motion xxx
        // please do motio dance
        if (tokens[i] == "do" && i + 2 < tokens.size() && tokens[i + 1] == "motion") {
            motion->pushTask("MOTIONSET:" + tokens[i + 2]);
            speaker->pushTask("do motion " + tokens[i + 2]);
            return true;
        }

        // learn motion xxx
        // please learn motion dance
        if (tokens[i] == "learn" && i + 2 < tokens.size() && tokens[i + 1] == "motion") {
            isLearningMode = true;
            motion->pushTask("LEARNMOTION:" + tokens[i + 2]);
            speaker->pushTask("learn motion " + tokens[i + 2]);
            return true;
        }

        // this is xxx
        // this is apple
        if (tokens[i] == "this" && i + 2 < tokens.size() && tokens[i + 1] == "is") {
            camera->pushTask("LEARNOBJ:" + tokens[i + 2]);
            speaker->pushTask("learn object " + tokens[i + 2]);
            return true;
        }

        if (text.find("done") != std::string::npos || text.find("stop") != std::string::npos || text.find("finish") != std::string::npos) {
            isLearningMode = false;
            return true;
    }
    }

    return false; // 没找到

}

// btn意图分发
bool RobotBrain::btn_detected(const std::string& type, const std::string& data) {
    // 传来的数据是固定的，因此不需要反复修改
    // MOTION_LEARN MOTION_CONFIRM  VISION_LEARN  VISION_DETECT  VISION_UPDATE DO_MOTION
    // 就写if了屏幕的后续扩展应该不多
    if (type == "MOTION_LEARN") {
        isLearningMode = true;
        motion->pushTask("LEARNMOTION:" + data);
        speaker->pushTask("learn motion " + data);
        _lastlearnmotion = data; // 记录正在学习的动作
        return true;
    }else if (type == "MOTION_CONFIRM") {
        isLearningMode = false;
        motion->pushTask("CONFIRM");
        speaker->pushTask("i know how to " + _lastlearnmotion);
        _lastlearnmotion = "None";
        return true;
    }else if (type == "VISION_LEARN") {
        camera->pushTask("LEARNOBJ:" + data);
        speaker->pushTask("learn object " + data);
        return true;
    }else if (type == "VISION_DETECT") {
        camera->pushTask("FINDOBJ:" + data);
        speaker->pushTask("find object " + data);
        return true;
    }else if (type == "VISION_UPDATE") {
        camera->pushTask("UPDATEBG");
        speaker->pushTask("update vision");
        return true;
    }else if (type == "DO_MOTION") {
        motion->pushTask("DOMOTION:" + data);
        return true;
    }else if (type == "MOTION_SET") {
        motion->pushTask("MOTIONSET:" + data);
        speaker->pushTask("do motion" + data);
        return true;
    }else if (type == "RESET") {
        motion->pushTask("RESET");
        speaker->pushTask("reset now");
        return true;
    }else if (type == "STOP") {
        motion->pushTask("STOP");
        speaker->pushTask("stop now");
        return true;
    }


    return false;
}
