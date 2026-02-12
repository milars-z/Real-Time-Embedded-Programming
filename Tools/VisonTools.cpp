#include "VisonTools.hpp"

#include <iostream>
#include <vector>
#include <string>

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
std::string find_alsa_device(const std::string& keyword, bool is_capture) {
    int card = -1;
    char *name = nullptr;
    
    while (snd_card_next(&card) == 0 && card >= 0) {
        // get device long name
        if (snd_card_get_longname(card, &name) == 0) {
            std::string longName(name);
            free(name);
            
            // find keyword in long name
            if (longName.find(keyword) != std::string::npos) {
                
                return (is_capture ? "plughw:" : "hw:") + std::to_string(card) + ",0";
            }
        }
    }
    return ""; 
}