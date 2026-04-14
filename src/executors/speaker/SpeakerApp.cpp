#include "SpeakerApp.hpp"

#include "SpeakerEngine.hpp"
#include "Config.hpp" 
#include <iostream>

SpeakerExecutor::SpeakerExecutor(const std::string& path,std::shared_ptr<TaskMonitor> taskMonitor) 
:_taskMonitor(taskMonitor)
{
    // 初始化底层播放引擎
    speaker = std::make_unique<UsbSpeaker>(
        path, // device path
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
SpeakerExecutor::~SpeakerExecutor() {
    std::cout << "[SpeakerExecutor] destructor end" << std::endl;
}

std::string SpeakerExecutor::get_module_name(){
    return "Speaker";
}

// 阻塞退出
void SpeakerExecutor::_stop(){
    if(speaker){
        speaker->stop_thread();
        std::cout << "[Speaker] 内部线程已退出..." << std::endl;
    }
}

void SpeakerExecutor::_start(int core){
    speaker->start_thread(core);
    std::cout << "[Speaker] 内部线程已开启,绑定在core:" << core << std::endl;
}


void SpeakerExecutor::onExecute(const std::string& text) {
    if (!text.empty() && speaker) {
        // 调用底层speaker的播放接口
        speaker->play(text);
    }
}

void SpeakerExecutor::pinThread(int num){
    pinThreadToCore(this->worker, "SpeakerTask", num);
}