#include "Tools.hpp"
#include "BugCode.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#include <alsa/asoundlib.h>

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
void print_startup_banner() {

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


    auto status = [INNER_WIDTH](std::string mod, std::string stat) {
        std::string prefix = "> " + mod + " ";
        std::string suffix = " [ " + stat + " ]";
        // 计算中间需要填充多少个点
        int dot_count = INNER_WIDTH - prefix.length() - suffix.length();
        if (dot_count < 0) dot_count = 0;

        std::cout << "║ " << prefix << std::string(dot_count, '.') 
                  << suffix << " ║" << std::endl;
    };


    std::cout << "\033[1;36m"; 

    hr("╔", "═", "╗");
    line("COGNIALM ROBOTIC SYSTEM v1.2", true);
    hr("╠", "═", "╣");

    line("> [SYSTEM] Initializing Modular Components...");
    status("MOUTH  Speaker Device ", "OK");
    status("EAR  Microphone     ", "OK");
    status("EYE Camera Sensor  ", "OK");
    status("BODY Servo Controller", "OK");
    status("BRAIN nlu model", "OK");
    status("CONSOLE LVGL Screen Env", "OK");

    hr("╠", "═", "╣");
    line("[STATUS]  ALL SYSTEMS ARE NOMINAL. STARTING...", false);
    hr("╚", "═", "╝");

    std::cout << "\033[0m" << std::endl; 
}

