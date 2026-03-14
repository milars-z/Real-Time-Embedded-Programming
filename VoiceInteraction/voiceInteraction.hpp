#pragma once

#include "MicrophoneEngine.hpp"
#include "SpeakerEngine.hpp"
#include "nlu_test.hpp"
#include "ThreadSafeQueue.hpp" 

#include <vosk_api.h>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

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

private:
    // user input
    void audioCallback(const std::vector<short>& data);

    // userinput -> command
    void nluWorker();

    // userinput -> speaker reply
    void speakerWorker();

    // init state
    void stopInternal();

private:
    

    // ptr
    UsbMicrophone* mic = nullptr;
    UsbSpeaker* speaker = nullptr;
    NLUEngine* nlu = nullptr;
    
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
    
};