#include "SpeakerApp.hpp"

#include "SpeakerEngine.hpp"
#include "Config.hpp" 
#include "SystemCode.hpp"
#include <iostream>

#include <nlohmann/json.hpp>
#include <fstream>

SpeakerExecutor::SpeakerExecutor(std::atomic<int>& system_state ,const std::string& path, const std::string& text_path, std::shared_ptr<TaskMonitor> taskMonitor) 
:_taskMonitor(taskMonitor),_speaker_path(text_path)
{
    // 初始化底层播放引擎
    speaker = std::make_unique<UsbSpeaker>(
        path, // device path
        2,  // hardware settings
        0,   // language setting: 0 for English, 1 for Chinese
        _taskMonitor
    );

    if (speaker->open()) {
        std::cout << "[Speaker] 音频硬件已就绪" << std::endl;
    } else {
        std::cerr << "[Speaker] 无法打开音频设备: " << path << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    }

    if(loadLibrary()){
        std::cout << "[Speaker] text已就绪" << std::endl;
    }else {
        std::cerr << "[Speaker] 无法找到正确的text映射 " << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    } 
    if(loadVariable()){
        std::cout << "[Speaker] variable已就绪" << std::endl;
    }else {
        std::cerr << "[Speaker] 无法找到正确的variable映射 " << std::endl;
        system_state |= ERR_SPEAKER_INIT;
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
    if(!speaker) return;
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


bool SpeakerExecutor::loadLibrary(){


    std::ifstream file(_speaker_path);

    if (!file.is_open()) {
        std::cerr << "cant load speaker_text" << std::endl;
        return false;
    }else{
        nlohmann::json data;
        file >> data;
        for (auto& it : data.items()){
            std::string key = it.key();
            std::string en  = it.value()["en"];
            std::string zh  = it.value()["zh"];
            text_lib[key] = {en, zh};
        }
    }
    return true;
}

std::string SpeakerExecutor::getText(const std::string& key){
    std::string text;
    auto it = text_lib.find(key);
    if (it == text_lib.end()) {
        if (currentLang == "en"){
            return "please check speaker test";
        }else if( currentLang == "zh"){
            return "我不知道该说些什么";
        }
    }else{

        if(currentLang == "en"){
            text = it->second.en;
        }else if(currentLang == "zh"){
            text = it->second.zh;
        }

        std::string target = "{host_name}";
        std::string actual_name = _host_name;
        size_t pos = text.find(target);
        while (pos != std::string::npos) {
            text.replace(pos, target.length(), actual_name);
            pos = text.find(target, pos + actual_name.length());
        }

        target = "{robot_name}";
        actual_name = _robot_name;
        pos = text.find(target);
        while (pos != std::string::npos) {
            text.replace(pos, target.length(), actual_name);
            pos = text.find(target, pos + actual_name.length());
        }

    }
    return text;
}

void SpeakerExecutor::setLanguage(const std::string& lang){

    bool is_success = false;

    if (lang == "zh"){
        currentLang = "zh";
    }else if ( lang == "en"){
        currentLang = "en";
    }else{
        currentLang = "en";
    }

    // is_success = loadLibrary();

    // is_success &= loadVariable();

    // return is_success;

}

void SpeakerExecutor::setVariable(const std::string& key, const std::string& value){

    if( key == "host_name" ){
        _host_name = value;
    }else if( key == "robot_name" ){
        _robot_name = value;
    }else{
        std::cout << "[SpeakerAPP] fail set variable" << std::endl;
    }

}

bool SpeakerExecutor::loadVariable(){

    config_var _var;
    _var = screen_get_var();
    currentLang = _var.lang;
    if( (_var.host != "Error_value") && (_var.robot != "Error_value")){
        _host_name = _var.host;
        _robot_name = _var.robot;
        return true;
    }else{
        return false;
    }

}
