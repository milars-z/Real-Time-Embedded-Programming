#include "MicrophoneApp.hpp"

#include "MicrophoneEngine.hpp"
#include "Config.hpp"
#include "Tools.hpp"  
#include "SystemCode.hpp"
#include <vosk_api.h>      
#include <iostream>


VoiceProducer::VoiceProducer(std::atomic<int>& system_state, const std::string& path, TextCallback callback, std::shared_ptr<TaskMonitor> taskMonitor) 
    : onTextReady(callback),_taskMonitor(taskMonitor) {
    
    // ASR 模型加载
    model = vosk_model_new(Config::Path::VOSK_MODEL_DIR.c_str());
    if (!model) {
        std::cerr << "[Error][MicrophoneApp] Failed to load Vosk model." << std::endl;
        system_state |= ERR_MICMODE_INIT;
        return;
    }
    recognizer = vosk_recognizer_new(model, 16000.0);

    // 初始化麦克风模块
    mic = std::make_unique<UsbMicrophone>(path, 16000, 1);
    if (!mic->open()) {
        system_state |= ERR_MIC_INIT;
        mic = nullptr;
        std::cerr << "[Error][MicrophoneApp] Failed to open microphone device." << std::endl;
    }
}

VoiceProducer::~VoiceProducer() {
    // stop(); 
    
    if (recognizer) {
        vosk_recognizer_free(recognizer);
        recognizer = nullptr;
    }
    if (model) {
        vosk_model_free(model);
        model = nullptr;
    }
    std::cout << "[MicrophoneApp] Voice resources released." << std::endl;
}

void VoiceProducer::start() {
    if (!mic) return;
    std::cout << "[MicrophoneApp] Starting voice listening..." << std::endl;
    
    mic->start([this](std::vector<short> data) {
    
        _snddata.push(data);
        // if (vosk_recognizer_accept_waveform(recognizer, 
        //                                     (const char*)data.data(), 
        //                                     data.size() * sizeof(short))) {
        //     std::string result = vosk_recognizer_result(recognizer);
        //     std::string text = extractText(result); 
        //     if (!text.empty() && onTextReady) {
        //         onTextReady(text); // 触发回调给 RobotBrain
        //     }else{

        //     }
        // }
    });
}

void VoiceProducer::_start(int core){
    if(!mic) return;
    start_thread(core);
    mic->start_thread(core);
    std::cout << "[MicrophoneApp] Internal thread started, pinned to core:" << core << std::endl;
}


void VoiceProducer::stop() {
    if (mic) {
        mic->stop(); 
        stop_thread();
        std::cout << "[MicrophoneApp] Microphone stopped." << std::endl;
    }
}

void VoiceProducer::start_thread(int core){

    isrunning = true;
    voskThread = std::thread(&VoiceProducer::voskWorker, this);
    pinThreadToCore(voskThread, "VoskWorkThread", core);
}

void VoiceProducer::stop_thread(){

    isrunning = false;
    _snddata.stop();
    if (voskThread.joinable()) voskThread.join();

}

void VoiceProducer::voskWorker(){
    
    while(isrunning){
        std::vector<short> data;
        
        if (!_snddata.pop(data)) {
            break;
        }

        if (!recognizer) continue;

        if (vosk_recognizer_accept_waveform(recognizer, 
                                        (const char*)data.data(), 
                                        data.size() * sizeof(short))){
#ifdef TESTMODE
// 有内容，开始准备检测
        TaskEvent _taskevent;
        TaskDescribe _taskdescribe;
        _taskevent.moduleName = "Microphone";
        _taskevent.status = TaskStatus::STARTED;
        _taskdescribe.TaskType = "STT";
        _taskdescribe.Name = "None";
        _taskevent.result = bg;
        _taskevent.taskType = _taskdescribe;
        _taskevent.taskId = task_id++;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskMonitor->postEvent(_taskevent);
#endif
        std::string result = vosk_recognizer_result(recognizer);
        std::string text = extractText(result);

        if (!text.empty() && onTextReady){
#ifdef TESTMODE
                _taskevent.moduleName = "Microphone";
                _taskevent.status = TaskStatus::FINISHED;
                _taskdescribe.TaskType = "STT";
                _taskdescribe.Name = text ;
                _taskevent.result = bg;
                _taskevent.taskType = _taskdescribe;
                _taskevent.issuccessful = true;  
                _taskevent.timestamp = std::chrono::steady_clock::now();
                _taskMonitor->postEvent(_taskevent);
#endif
            onTextReady(text);
        }else{
#ifdef TESTMODE
                _taskevent.moduleName = "Microphone";
                _taskevent.status = TaskStatus::FINISHED;
                _taskdescribe.TaskType = "STT";
                _taskdescribe.Name = "None" ;
                _taskevent.result = bg;
                _taskevent.issuccessful = false; 
                _taskevent.taskType = _taskdescribe;
                _taskevent.timestamp = std::chrono::steady_clock::now();
                _taskMonitor->postEvent(_taskevent);
#endif
        }
                                        }
        
    }
}