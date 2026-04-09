#include "SpeakerApp.hpp"

#include "SpeakerEngine.hpp"
#include "config_voice.hpp" 
#include <iostream>

SpeakerExecutor::SpeakerExecutor(const std::string& path) {
    // 初始化底层播放引擎
    speaker = std::make_unique<UsbSpeaker>(
        path, // device path
        Config::Path::SPEAKER_MODELS, // model paths
        2,  // hardware settings
        0   // language setting: 0 for English, 1 for Chinese
    );

    if (speaker->open()) {
        std::cout << "[Speaker] 音频硬件已就绪" << std::endl;
    } else {
        std::cerr << "[Speaker] 无法打开音频设备: " << path << std::endl;
    }
}

// 暂时用不到结构函数
// 后续切换语言时或许需要在析构函数中清理资源
SpeakerExecutor::~SpeakerExecutor() = default;

void SpeakerExecutor::onExecute(const std::string& text) {
    if (!text.empty() && speaker) {
        // 调用底层speaker的播放接口
        speaker->play(text);
    }
}