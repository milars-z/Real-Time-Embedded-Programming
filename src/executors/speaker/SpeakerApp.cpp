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
    // Initialize the underlying playback engine
    speaker = std::make_unique<UsbSpeaker>(
        path, // device path
        2,  // hardware settings
        0,   // language setting: 0 for English, 1 for Chinese
        _taskMonitor
    );

    if (speaker->open()) {
        std::cout << "[Init][SpeakerApp] Audio hardware is ready" << std::endl;
    } else {
        std::cerr << "[Error][SpeakerApp] Failed to open the audio device: " << path << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    }

    if(loadLibrary()){
        std::cout << "[Init][SpeakerApp] Text library is ready" << std::endl;
    }else {
        std::cerr << "[Error][SpeakerApp] Failed to find the correct text mapping " << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    } 
    if(loadVariable()){
        std::cout << "[Init][SpeakerApp] Variables are ready" << std::endl;
    }else {
        std::cerr << "[Error][SpeakerApp] Failed to find the correct variable mapping " << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    }    
}

// The destructor is not needed for now
// Resource cleanup may be needed in the destructor when language switching is added later
SpeakerExecutor::~SpeakerExecutor() {
    std::cout << "[End][SpeakerApp] destructor end" << std::endl;
}

std::string SpeakerExecutor::get_module_name(){
    return "Speaker";
}

// Blocking shutdown
void SpeakerExecutor::_stop(){
    if(speaker){
        speaker->stop_thread();
        std::cout << "[End][SpeakerApp] Internal thread has exited..." << std::endl;
    }
}

void SpeakerExecutor::_start(int core){
    if(!speaker) return;
    speaker->start_thread(core);
    std::cout << "[Init][SpeakerApp] Internal thread started, bound to core:" << core << std::endl;
}


void SpeakerExecutor::onExecute(const std::string& text) {
    if (!text.empty() && speaker) {
        // Call the playback interface of the underlying speaker
        speaker->play(text);
    }
}

void SpeakerExecutor::pinThread(int num){
    pinThreadToCore(this->worker, "SpeakerTask", num);
}


bool SpeakerExecutor::loadLibrary(){


    std::ifstream file(_speaker_path);

    if (!file.is_open()) {
        std::cerr << "[Error][SpeakerApp]cant load speaker_text" << std::endl;
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
            return "I'm not sure what to say.";
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

}

void SpeakerExecutor::setVariable(const std::string& key, const std::string& value){

    if( key == "host_name" ){
        _host_name = value;
    }else if( key == "robot_name" ){
        _robot_name = value;
    }else{
        std::cerr << "[Error][SpeakerAPP] fail set variable" << std::endl;
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
