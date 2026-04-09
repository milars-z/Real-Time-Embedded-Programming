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

    if (isLearningMode) {
        processLearning(text);
        return;
    }

    // 调用 NLU 解析意图
    auto res = nlu->predict(text);
    
    std::cout << "[Brain] 意图识别: " << res.intent << " 参数: " << res.currentValue << std::endl;

    // if (res.intent == "do_motion") {
    //     speaker->pushTask("do motion");
    //     motion->pushTask(res.currentValue); // 执行特定动作
    // } 
    // else if (res.intent == "greet") {
    //     speaker->pushTask("hello");
    // }
}

void RobotBrain::handleUISignal(const std::string& type, const std::string& data) {
    // 处理来自 ScreenApp 的 UI 交互信号
    printf("[Brain] 收到 UI 信号: %s, 数据: %s\n", type.c_str(), data.c_str());
    
    if (type == "LEARN_START") {
        isLearningMode = true;
        speaker->pushTask("learn mode");
        camera->pushTask("VISION_LEARN:" + data); // 进入学习模式，传递物体名称

    } 
}

void RobotBrain::processLearning(const std::string& text) {
    printf("[Brain] 学习模式 - 正在记录特征: %s\n", text.c_str());
    
}