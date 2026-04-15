#include "cogniArm.hpp"

#include "SpeakerApp.hpp"
#include "MotionApp.hpp"
#include "CameraApp.hpp"
#include "RobotBrain.hpp"
#include "ScreenApp.hpp"
#include "MicrophoneApp.hpp" 
#include "Config.hpp" 
#include "Tools.hpp"    
#include "TaskMonitor.hpp" 
#include "TaskSupervisor.hpp"

#include <iostream>
#include <unistd.h> 

extern std::atomic<bool> _exit_signal;

RobotSystem::RobotSystem() = default;

RobotSystem::~RobotSystem() = default;

bool RobotSystem::init() {
    std::cout << "[CogniArm] Initializing hardware..." << std::endl;

    // 硬件路径查找
    std::string speaker_path = find_alsa_device(Config::Hardware::SPEAKER_NAME);
    std::string mic_path = find_alsa_device(Config::Hardware::MIC_NAME);

    // 判定
    // 其实有点多余，麦克风不开初始化不了
    if (speaker_path.empty() || mic_path.empty()) {
        std::cerr << "[Fatal][CogniArm] Audio hardware device not found!" << std::endl;
        return false;
    }

    // 新增一个全局监控者，该指针传入三个执行者模块用来返回结果
    globalMonitor = std::make_shared<TaskMonitor>();

    // 初始化执行层
    std::cout << "[CogniArm] Initializing Speaker..." << std::endl;
    speaker = std::make_shared<SpeakerExecutor>(speaker_path,globalMonitor);
    std::cout << "[CogniArm] Initializing Motion..." << std::endl;
    motion  = std::make_shared<MotionExecutor>(globalMonitor);
    std::cout << "[CogniArm] Initializing Camera..." << std::endl;
    camera  = std::make_shared<CameraExecutor>(globalMonitor);

    // 初始化brain逻辑层
    brain = std::make_unique<RobotBrain>(speaker, motion, camera, globalMonitor);

    // 初始化输入层，绑定输入回调到 brain
    // 语音输入
    voiceIn = std::make_unique<VoiceProducer>(mic_path, [this](std::string text) {
        if(brain) brain->handleIncomingText(text);
    }, globalMonitor);

    // 屏幕输入
    screenIn = std::make_unique<ScreenProducer>(camera, [this](std::string t, std::string d){
        if(brain) brain->handleUISignal(t, d);
    });

    // supervisor
    supervisor = std::make_unique<TaskSupervisor>(globalMonitor,motion,speaker);

    return true;
}

// 启动！
// 后续可能要再维护一下
void RobotSystem::start() {
    if (isRunning) return;

    _exit_signal = false;
    isRunning = true;

    

    voiceIn->start();
    screenIn->start();

    std::cout << "[CogniArm] Modular system started." << std::endl;

    // 优先开启底层执行者线程
    std::cout << "[CogniArm] Starting executor threads..." << std::endl;
    speaker->_start(3);
    motion->_start(1);
    camera->_start(2);

    // 启动任务线程
    std::cout << "[CogniArm] Starting task threads..." << std::endl;
    speaker->start();
    motion->start();
    camera->start();

    std::cout << "[CogniArm] Binding task threads..." << std::endl;
    speaker->pinThread(3);
    motion->pinThread(1);
    camera->pinThread(2);
    
    std::cout << "[CogniArm] Starting producer threads..." << std::endl;
    voiceIn->_start(3);

    // 启动检测线程
    supervisor->start_thread(0);


    std::cout << "[CogniArm] Initialization complete." << std::endl;

    while (isRunning) {

        if (_exit_signal) {
            break;
        }

        // UI 更新循环
        uint32_t next_ms = screenIn->update();
        
        // 限制帧率最高为33fps左右
        next_ms = std::max(1u, std::min(next_ms, 30u));
        usleep(next_ms * 1000);
    }
    stop();
}

// 停止系统，清空资源
void RobotSystem::stop() {
    if (!isRunning) return;
    isRunning = false;

    std::cout << "[CogniArm] Stopping system..." << std::endl;
    supervisor->stop_thread();
    // 停止输入任务
    voiceIn->stop();
    screenIn->stop();

    // 停止输出任务
    // 退出任务线程
    motion->stop();
    speaker->stop();
    camera->stop();

    // 退出底部线程
    motion->_stop();
    speaker->_stop();
    camera->_stop();

    std::cout << "[CogniArm] Threads exited safely... destroying resources..." << std::endl;
}

