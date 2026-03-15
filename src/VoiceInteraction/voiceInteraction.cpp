#include "config_voice.hpp"
#include "VisonTools.hpp"
#include "voiceInteraction.hpp"
#include <iostream>
#include <chrono>


// 构造函数
RobotCore::RobotCore() : isRunning(false) {
}

// 析构函数 -停止所有相关模块运行
RobotCore::~RobotCore() {
    stop(); 
}

// 初始化
bool RobotCore::init() {

    // Find device name
    std::string speaker_path = find_alsa_device(Config::Hardware::SPEAKER_NAME);
    std::string mic_path = find_alsa_device(Config::Hardware::MIC_NAME);
    if (speaker_path.empty() || mic_path.empty()) {
        std::cerr << "[Error] Devices not found!" << std::endl;
        return false;
    }

    // Init NLU
    nlu = new NLUEngine(Config::Path::NLU_MODEL_DIR);
    if (!nlu->init()) {
        std::cerr << "[Error] NLUEngine failed." << std::endl;
        goto fail;
    }

    // Init Vosk
    voskModel = vosk_model_new(Config::Path::VOSK_MODEL_DIR.c_str());
    if (!voskModel) {
        std::cerr << "[Error] Vosk model load failed" << std::endl;
        goto fail;
    }
    recognizer = vosk_recognizer_new(voskModel, 16000.0);
    if(!recognizer){
        std::cerr << "[Error] vosk_recognizer load failed" << std::endl;
        goto fail;
    }

    // Init Speaker
    speaker = new UsbSpeaker(speaker_path, Config::Path::SPEAKER_MODELS, 2, 0);
    if (!speaker->open()) {
        std::cerr << "[Error] Speaker open failed" << std::endl;
        goto fail;
    }

    // Init Microphone
    mic = new UsbMicrophone(mic_path, 16000, 1);
    if (!mic->open()) {
        std::cerr << "[Error] Mic open failed" << std::endl;
        goto fail;
    }

    std::cout << "[Init] System initialized successfully." << std::endl;
    return true;

fail:
    if(mic)       { delete mic; mic = nullptr;}
    if(speaker)   { delete speaker; speaker = nullptr;}
    if(recognizer){ vosk_recognizer_free(recognizer); recognizer = nullptr;}
    if(voskModel) { vosk_model_free(voskModel); voskModel = nullptr;}
    if(nlu)       { delete nlu; nlu = nullptr; }
    return false;
}

// 启动
bool RobotCore::start() {
    if (isRunning) return true;
    isRunning = true;

    // 重置队列状态
    textInputQueue.reset();
    speechOutputQueue.reset();

    try{
        nluThread = std::thread(&RobotCore::nluWorker,this);
        speakerThread = std::thread(&RobotCore::speakerWorker,this);
        pinThreadToCore(nluThread,"nlu", 3);
        pinThreadToCore(speakerThread,"speaker", 3);
    }catch (const std::system_error& e){
        std::cerr<<"[Fatat] Thread speaker and nlu error: " << e.what()<< std::endl;
        stopInternal();
        return false;
    }

    bool micStarted = mic->start([this] (const std::vector<short>& data){
        this -> audioCallback(data);
    });
    if (!micStarted){
        stopInternal();
        return false;
    }

    std::cout << "[System] Started." << std::endl;
    return true;
}

// 停止
void RobotCore::stop() {
    if (!isRunning) return;
    
    std::cout << "[System] Stopping..." << std::endl;
    isRunning = false;

    // 停止麦克风
    if (mic) mic->stop();

    // 停止队列
    textInputQueue.stop();
    speechOutputQueue.stop();

    // 停止线程
    if (nluThread.joinable()) nluThread.join();
    if (speakerThread.joinable()) speakerThread.join();

    // 关闭ALSA
    if (speaker) speaker->stop(); 

    // 释放vosk
    if (recognizer) { vosk_recognizer_free(recognizer); recognizer = nullptr; }
    if (voskModel)  { vosk_model_free(voskModel); voskModel = nullptr; }

    // 删除对象
    delete mic; mic = nullptr;
    delete speaker; speaker = nullptr;
    delete nlu; nlu = nullptr;

    std::cout << "[System] Stopped and resources cleaned." << std::endl;
}

bool RobotCore::running() const {
    return isRunning;
}

// use in RobotCore::start
void RobotCore::stopInternal() {
    isRunning = false; 
    textInputQueue.stop();
    speechOutputQueue.stop();
    if (nluThread.joinable()) nluThread.join();
    if (speakerThread.joinable()) speakerThread.join();
    if(mic) mic->stop();
}


// microphone callback
void RobotCore::audioCallback(const std::vector<short>& data) {
    if (vosk_recognizer_accept_waveform(recognizer, 
                                       (const char*)data.data(), 
                                       data.size() * sizeof(short))) {
        
        std::string result = vosk_recognizer_result(recognizer);
        std::string text = extractText(result); 

        if (!text.empty()) {
            std::cout << "[Mic] Heard: " << text << std::endl;
            textInputQueue.push(text); 
        }
    }
}

// nlu thread
void RobotCore::nluWorker() {
    std::string text;
    while (isRunning) {
        // // wait for audio--textInputQueue
        if (textInputQueue.pop(text)) {

            nlu_output nlu_outp = nlu->predict(text);
            
            std::string responseText = "";

            // 后续连接其他类进行语义处理
            if (nlu_outp.intent == "greet") {
                responseText = "hello";
            } else if (nlu_outp.intent == "bye") {
                responseText = "bye";
            } else if (nlu_outp.intent == "check_host_name") {
                responseText = "hello, milars";
            } else if (nlu_outp.intent == "check_rot_name") {
                responseText = "hello, i am cognitive robot arm";
            } else if (nlu_outp.intent == "learn_motion") {
                responseText = "i am learning how to " + nlu_outp.currentValue;
            } else if (nlu_outp.intent == "learn_obj") {
                responseText = "i am learning what is " + nlu_outp.currentValue;
            } else if (nlu_outp.intent == "do_motion") {
                responseText = "i am doing " + nlu_outp.currentValue;
            } else if (nlu_outp.intent == "find_obj") {
                responseText = "i am finding " + nlu_outp.currentValue;
            } 

            if (!responseText.empty()) {
                std::cout << "[NLU] Intent: " << nlu_outp.intent << " -> Reply: " << responseText << std::endl;
                speechOutputQueue.push(responseText);
            }
        } else {
            break; 
        }
    }
}


// speaker thread
void RobotCore::speakerWorker() {
    std::string textToPlay;
    while (isRunning) {
        // wait for NLU--speechOutputQueue
        if (speechOutputQueue.pop(textToPlay)) {
            speaker->play(textToPlay);
        } else {
            break;
        }
    }
}