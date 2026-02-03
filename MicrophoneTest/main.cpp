#include "MicrophoneEngine.hpp"
#include <vosk_api.h>
#include <iostream>
#include <vector>
#include <string>

// get text from json doc provided by vosk
std::string extractText(const std::string& json) {
    size_t start = json.find("\"text\" : \"");
    if (start == std::string::npos) return "";
    start += 10;
    size_t end = json.find("\"", start);
    return json.substr(start, end - start);
}

int main() {
    // load language model
    VoskModel *model = vosk_model_new("model");
    if (!model) {
        std::cerr << "Error: Could not load English model!" << std::endl;
        return -1;
    }
    
    // create a Recognizer
    // 16K is better for English rocognize
    VoskRecognizer *recognizer = vosk_recognizer_new(model, 16000.0);

    // Init microphone device
    UsbMicrophone mic("plughw:2,0", 16000, 1);
    
    if (!mic.open()) {
        return -1;
    }

    std::cout << "\n>>> English Voice Recognition Started <<<" << std::endl;
    std::cout << "You can speak English now..." << std::endl;

    // stt
    // this part will change to a recognise class
    auto audioHandler = [recognizer](const std::vector<short>& data) {
        if (vosk_recognizer_accept_waveform(recognizer, 
                                           (const char*)data.data(), 
                                           data.size() * sizeof(short))) {
            
            // get final answer
            std::string result = vosk_recognizer_result(recognizer);
            std::string text = extractText(result);
            
            if (!text.empty() && text != " ") {
                std::cout << "\r[Recognized]: " << text << std::endl;
                
                // some other function ... 
                // better to use a class to manage
                if (text == "stop" || text == "exit") {
                    std::cout << "System stopping..." << std::endl;
                    // like mic.stop()
                    // but it dont know mic
                }
            }
        } else {
            // Partial Result
            // std::string partial = vosk_recognizer_partial_result(recognizer);
            // std::cout << "\r[Listening...]: " << extractText(partial) << std::flush;
            // not use
        }
    };

    
    mic.start(audioHandler);

    // keep doing main thread 
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // free
    mic.stop();
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);

    return 0;
}