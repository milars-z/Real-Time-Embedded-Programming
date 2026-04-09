#include "cogniArm.hpp"

#include "SpeakerApp.hpp"
#include "MotionApp.hpp"
#include "CameraApp.hpp"
#include "RobotBrain.hpp"
#include "ScreenApp.hpp"
#include "MicrophoneApp.hpp" 
#include "config_voice.hpp" 
#include "VisonTools.hpp"     

#include <iostream>
#include <unistd.h> 

extern std::atomic<bool> _exit_signal;

RobotSystem::RobotSystem() = default;

RobotSystem::~RobotSystem() {
    stop();
}

bool RobotSystem::init() {
    std::cout << "[System] 正在初始化硬件..." << std::endl;

    // 硬件路径查找
    std::string speaker_path = find_alsa_device(Config::Hardware::SPEAKER_NAME);
    std::string mic_path = find_alsa_device(Config::Hardware::MIC_NAME);

    // 判定
    // 其实有点多余，麦克风不开初始化不了
    if (speaker_path.empty() || mic_path.empty()) {
        std::cerr << "[Fatal] 找不到音频硬件设备！" << std::endl;
        return false;
    }

    // 初始化执行层
    speaker = std::make_shared<SpeakerExecutor>(speaker_path);
    motion  = std::make_shared<MotionExecutor>();
    camera  = std::make_shared<CameraExecutor>();

    // 初始化brain逻辑层
    brain = std::make_unique<RobotBrain>(speaker, motion, camera);

    // 初始化输入层，绑定输入回调到 brain
    // 语音输入
    voiceIn = std::make_unique<VoiceProducer>(mic_path, [this](std::string text) {
        if(brain) brain->handleIncomingText(text);
    });

    // 屏幕输入
    screenIn = std::make_unique<ScreenProducer>(camera, [this](std::string t, std::string d){
        if(brain) brain->handleUISignal(t, d);
    });

    return true;
}

// 启动！
// 后续可能要再维护一下
void RobotSystem::start() {
    if (isRunning) return;
    isRunning = true;

    // 启动各个执行器的线程
    speaker->start();
    motion->start();
    camera->start();

    voiceIn->start();
    screenIn->start();

    std::cout << "[System] 模块化系统已启动" << std::endl;

    while (isRunning) {

        if (_exit_signal) {
            stop();
            break;
        }

        // UI 更新循环
        uint32_t next_ms = screenIn->update();
        
        // 限制帧率最高为33fps左右
        next_ms = std::max(1u, std::min(next_ms, 30u));
        usleep(next_ms * 1000);
    }
}

// 停止系统，清空资源
void RobotSystem::stop() {
    if (!isRunning) return;
    isRunning = false;

    std::cout << "[System] 正在停止系统..." << std::endl;
    voiceIn->stop();
    screenIn->stop();
    motion->stop();
    speaker->stop();
    camera->stop();
}

