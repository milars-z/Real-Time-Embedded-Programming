#pragma once

#include "MicrophoneEngine.hpp"
#include "SpeakerEngine.hpp"
#include "NluHandle.hpp"
#include "MotionHandle.hpp"
#include "CameraHandle.hpp"
#include "ThreadSafeQueue.hpp" 
#include "MotionManager.hpp"

#include <vosk_api.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>


enum class RobotIntent {
    DO_MOTION,
    LEARN_MOTION,
    FIND_OBJ,
    LEARN_OBJ,
    GREET,
    NORMAL,
    SERVO_INIT,
    UNKNOWN
};

struct IntentContext {
    RobotIntent intent;
    std::string value; 
};

class RobotCore {
public:
    RobotCore();
    ~RobotCore();

    // 初始化硬件和模型
    bool init();

    // 启动所有线程
    bool start();

    // 停止所有线程并释放资源
    void stop();

    // 检查是否正在运行
    bool running() const;

    // 外界调用Cam接口
    // 用来推流测试
    CameraHandle* getCamHandle() const;

private:
    // user input
    void audioCallback(const std::vector<short>& data);

    // userinput -> command
    void nluWorker();

    // userinput -> speaker reply
    void speakerWorker();

    // init state
    void stopInternal();

    void handleDoMotion(const std::string& val, std::string& response);
    void handleLearnMotion(const std::string& val, std::string& response);
    void handleFindObj(const std::string& val, std::string& response);
    void handleLearnObj(const std::string& val, std::string& response);
    void handleNormal(const std::string& text, std::string& response);
    RobotIntent mapToIntent(const std::string& nlu_intent);

    // 外界调用函数
    // 学习状态处理，传入motion名称，开始学习
    void processLearningInput(const std::string& text, std::string& response);

    // 内部处理
    // 用来处理motion string，将motionstring中的joint 提取，并生成MotionSet保存
    // 后续或许要追加motion的角度，例如将base旋转xx角度，所以输入时motion Task
    // motion task中的速度，角度，相对方式暂时用默认值
    bool saveMotionSet(std::string motionName, std::vector<MotionTask>& motionSet);


    void get_motion_name_from_text(const std::string& text, std::string& val);

private:
    

    // ptr
    UsbMicrophone* mic = nullptr;
    UsbSpeaker* speaker = nullptr;
    NLUEngine* nlu = nullptr;
    CameraHandle* cam = nullptr;
    
    // Vosk 
    VoskModel* voskModel = nullptr;
    VoskRecognizer* recognizer = nullptr;

    // Queue -- use cv.wait
    ThreadSafeQueue<std::string> textInputQueue;   // Mic -> NLU
    ThreadSafeQueue<std::string> speechOutputQueue;// NLU -> Speaker

    // thread
    std::thread nluThread;
    std::thread speakerThread;
    std::atomic<bool> isRunning;
    
    MotionHandle motionHandle;

    MotionManager motionManager;

    bool _isLearningMode = false;

    // learn motion process temp storage
    std::string _currentLearningName = "";
    std::vector<MotionTask> _tempTasks; 
    Joint _currentJoint = Joint::Base;  


};