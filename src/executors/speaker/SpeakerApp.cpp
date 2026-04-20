#include "SpeakerApp.hpp"

#include "SpeakerEngine.hpp"
#include "Config.hpp" 
#include "SystemCode.hpp"
#include <iostream>

#include <nlohmann/json.hpp>
#include <fstream>

/**
 * @brief Constructor for SpeakerExecutor, initializes speaker hardware and loads text libraries
 * @param system_state Reference to system state atomic variable
 * @param path Audio device path
 * @param text_path Path to text library file
 * @param taskMonitor Shared pointer to task monitor
 */
SpeakerExecutor::SpeakerExecutor(std::atomic<int>& system_state ,const std::string& path, const std::string& text_path, std::shared_ptr<TaskMonitor> taskMonitor) 
:_taskMonitor(taskMonitor),_speaker_path(text_path)
{
    // init speaker engine
    speaker = std::make_unique<UsbSpeaker>(
        path, // device path
        2,  // hardware settings
        0,   // language setting: 0 for English, 1 for Chinese
        _taskMonitor
    );

    if (speaker->open()) {
        std::cout << "[Init][SpeakerApp] Audio hardware is ready" << std::endl;
    } else {
        std::cerr << "[Error][SpeakerApp] Failed to open audio device: " << path << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    }

    if(loadLibrary()){
        std::cout << "[Init][SpeakerApp] text is ready" << std::endl;
    }else {
        std::cerr << "[Error][SpeakerApp] Failed to find correct text mapping" << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    } 
    if(loadVariable()){
        std::cout << "[Init][SpeakerApp] variable is ready" << std::endl;
    }else {
        std::cerr << "[Error][SpeakerApp] Failed to find correct variable mapping" << std::endl;
        system_state |= ERR_SPEAKER_INIT;
    }    
}

/**
 * @brief Destructor for SpeakerExecutor
 * Currently is not used, may clean up resources when switching languages in the future
 */
SpeakerExecutor::~SpeakerExecutor() {
    std::cout << "[End][SpeakerApp] destructor end" << std::endl;
}

/**
 * @brief Get the module name
 * @return Module name as string
 */
std::string SpeakerExecutor::get_module_name(){
    return "Speaker";
}

/**
 * @brief Stop the internal thread with blocking
 */
void SpeakerExecutor::_stop(){
    if(speaker){
        speaker->stop_thread();
        std::cout << "[End][SpeakerApp] Internal thread exited..." << std::endl;
    }
}

/**
 * @brief Start the internal thread
 * @param core CPU core number to pin the thread to
 */
void SpeakerExecutor::_start(int core){
    if(!speaker) return;
    speaker->start_thread(core);
    std::cout << "[Init][SpeakerApp] Internal thread started, pinned to core:" << core << std::endl;
}


void SpeakerExecutor::onExecute(const std::string& text) {
    if (!text.empty() && speaker) {
        // use speaker interface
        speaker->play(text);
    }
}

/**
 * @brief Pin the worker thread to a specific CPU core
 * @param num CPU core number to pin to
 */
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

/**
 * @brief Get text from library based on key and current language
 * @param key Text key to look up
 * @return Text string in current language
 */
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

/**
 * @brief Set the current language for text output
 * @param lang Language code ("zh" for Chinese, "en" for English)
 */
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

/**
 * @brief Set variable value for text substitution
 * @param key Variable key ("host_name" or "robot_name")
 * @param value Value to set
 */
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
