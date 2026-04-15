#include "MicrophoneApp.hpp"

#include "MicrophoneEngine.hpp"
#include "Config.hpp"
#include "Tools.hpp"  
#include <vosk_api.h>      
#include <iostream>


VoiceProducer::VoiceProducer(const std::string& path, TextCallback callback, std::shared_ptr<TaskMonitor> taskMonitor) 
    : onTextReady(callback),_taskMonitor(taskMonitor) {
    
    // ASR 模型加载
    model = vosk_model_new(Config::Path::VOSK_MODEL_DIR.c_str());
    if (!model) {
        std::cerr << "[Error][MicrophoneApp] Failed to load Vosk model." << std::endl;
        return;
    }
    recognizer = vosk_recognizer_new(model, 16000.0);

    // 初始化麦克风模块
    mic = std::make_unique<UsbMicrophone>(path, 16000, 1);
    if (!mic->open()) {
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
    
    mic->start([this](const std::vector<short>& data) {
        if (!recognizer) return;

        if (vosk_recognizer_accept_waveform(recognizer, 
                                            (const char*)data.data(), 
                                            data.size() * sizeof(short))) {

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

            if (!text.empty() && onTextReady) {
                
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

                onTextReady(text); // 触发回调给 RobotBrain
         
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
    });
}

void VoiceProducer::_start(int core){
    mic->start_thread(core);
    std::cout << "[MicrophoneApp] Internal thread started, pinned to core:" << core << std::endl;
}


void VoiceProducer::stop() {
    if (mic) {
        mic->stop(); 
        std::cout << "[MicrophoneApp] Microphone stopped." << std::endl;
    }
}