#include "Tools.hpp"
#include "BugCode.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <fstream>

#include <alsa/asoundlib.h>

#include "system_config.hpp"

std::string extractText(const std::string& json) {
    size_t start = json.find("\"text\" : \"");
    if (start == std::string::npos) return "";
    start += 10;
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    return json.substr(start, end - start);
}

// is_capture: true for microphone, false for speaker
std::string find_alsa_device(const std::string& keyword) {
    int card = -1;
    char *name = nullptr;
    
    while (snd_card_next(&card) == 0 && card >= 0) {
        // get device long name
        if (snd_card_get_longname(card, &name) == 0) {
            std::string longName(name);
            free(name);
            
            // find keyword in long name
            if (longName.find(keyword) != std::string::npos) {
                
                return "plughw:" + std::to_string(card) + ",0";
            }
        }
    }
    return ""; 
}

// tool for multithread
void pinThreadToCore(std::thread &th, std::string thread_name, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error pinning thread to core " << core_id << std::endl;
    } else {
        std::cout << thread_name << "Thread bound to Core " << core_id << std::endl;
    }
}

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>


// wrote by AI
// 后续有时间追加不同状态的banner
void print_startup_banner(SystemConfig cfg) {

    const int WIDTH = 64; 

    const int INNER_WIDTH = WIDTH - 4; 


    auto hr = [WIDTH](std::string left, std::string mid, std::string right) {
        std::cout << left;
        for (int i = 0; i < WIDTH - 2; ++i) std::cout << mid;
        std::cout << right << std::endl;
    };


    auto line = [INNER_WIDTH](std::string text, bool center = false) {
        int text_len = text.length(); 
        int left_pad = 0, right_pad = 0;

        if (center) {
            left_pad = (INNER_WIDTH - text_len) / 2;
            right_pad = INNER_WIDTH - text_len - left_pad;
        } else {
            left_pad = 0;
            right_pad = INNER_WIDTH - text_len;
        }

        std::cout << "║ " << std::string(left_pad, ' ') << text 
                  << std::string(right_pad, ' ') << " ║" << std::endl;
    };


    auto status = [INNER_WIDTH](std::string mod, std::string stat, bool iserror = false) {
        std::string prefix = "> " + mod + " ";
        std::string suffix = " [ " + stat + " ]";
        // 计算中间需要填充多少个点
        int dot_count = INNER_WIDTH - prefix.length() - suffix.length();
        if (dot_count < 0) dot_count = 0;

        if (iserror){
            std::cout << "║ " << prefix << std::string(dot_count, '.') << "\033[31m" << suffix << "\033[1;36m" << " ║" << std::endl;
        }else{
            std::cout << "║ " << prefix << std::string(dot_count, '.') 
                  << suffix << " ║" << std::endl;
        }
        
    };


    std::cout << "\033[1;36m"; 

    hr("╔", "═", "╗");
    line("COGNIALM ROBOTIC SYSTEM v1.2", true);
    hr("╠", "═", "╣");

    line("> [SYSTEM] Initializing Modular Components...");
    if (cfg.enableSpeaker){
        status("MOUTH  Speaker Device ", "OK");
    }else{
        status("MOUTH  Speaker Device ", "DISABLE" , true);
    }
    if (cfg.enableMicrophone){
        status("EAR  Microphone     ", "OK");
    }else{
        status("EAR  Microphone     ", "DISABLE" , true);
    }
    if (cfg.enableCamera){
        status("EYE Camera Sensor  ", "OK");
    }else{
        status("EYE Camera Sensor  ", "DISABLE" , true);
    }
    if (cfg.enableMotion){
        status("BODY Servo Controller", "OK");
    }else{
        status("BODY Servo Controller", "DISABLE" , true);
    }
    if (cfg.enableNlu){
        status("BRAIN nlu model", "OK");
    }else{
        status("BRAIN nlu model", "DISABLE" , true);
    }
    if (cfg.enableScreen){
        status("CONSOLE LVGL Screen Env", "OK");
    }else{
        status("CONSOLE LVGL Screen Env", "DISABLE" , true);
    }
    
    hr("╠", "═", "╣");
    line("[STATUS]  ALL SYSTEMS ARE NOMINAL. STARTING...", false);
    hr("╚", "═", "╝");

    std::cout << "\033[0m" << std::endl; 
}

config_var screen_get_var(){

    std::ifstream file(Config::Path::VARIABLE_NAME);

    config_var _var;
    _var.lang = "en";
    _var.host = "Error_value";
    _var.robot = "Error_value";

    if (!file.is_open()) {
        std::cerr << "cant load speaker_text" << std::endl;
        return _var;
    }else{
        
        std::string language;
        std::string line;
        std::string currentSection = "";

        std::getline(file, language);
        if(_var.lang != "en" && _var.lang != "zh"){
            _var.lang = "en";
        }

        while (std::getline(file, line)){
            if (line.empty()) continue; 
            if(_var.lang == "en"){
                if (line == "en"){
                    std::getline(file, _var.host);
                    std::getline(file, _var.robot);
                }
            }else if(_var.lang == "zh"){
                if (line == "zh"){
                    std::getline(file, _var.host);
                    std::getline(file, _var.robot);
                }
            
            }
        }
        
    }
    return _var;
}

void saveVariablesToFile(const std::string& lang, const std::string& host, const std::string& robot ) {

    std::ofstream file(Config::Path::VARIABLE_NAME);
    
    if (file.is_open()) {

        file << lang << std::endl;

        file << "en" << std::endl;
        file << host << std::endl;   
        file << robot << std::endl;  
        
        file << "zh" << std::endl;
        file << host << std::endl;   
        file << robot << std::endl;  
        
        file.close();
    } else {
        std::cout << "[SpeakerAPP] fail to open file for writing" << std::endl;
    }
}

