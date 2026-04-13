#include "MicrophoneApp.hpp"

#include "MicrophoneEngine.hpp"
#include "config_voice.hpp"
#include "VisonTools.hpp"  
#include <vosk_api.h>      
#include <iostream>

VoiceProducer::VoiceProducer(const std::string& path, TextCallback callback) 
    : onTextReady(callback) {
    
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
            
            std::string result = vosk_recognizer_result(recognizer);
            
            std::string text = extractText(result); 

            if (!text.empty() && onTextReady) {
                onTextReady(text); // 触发回调给 RobotBrain
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