#include "MicrophoneEngine.hpp"
#include "SpeakerEngine.hpp"   
#include "VisonTools.hpp"

#include <vosk_api.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>

std::atomic<bool> keepRunning(true);

ModelPaths myModels = {
    "../../SpeakerTest/model/vits-piper-en_GB-cori-medium-int8" ,
    "../../SpeakerTest/model/vits-piper-zh_CN-huayan-medium"      
};

int main() {
    
    // INIT parts
    // GET SPEAKER AND MICROPHONE NAME
    // card 2: UACDemoV10 [UACDemoV1.0], device 0: USB Audio [USB Audio]
    std::string speaker_path = find_alsa_device("UACDemo");
    if (speaker_path.empty()) {
        std::cerr << "Error: Could not find speaker device!" << std::endl; 
        return -1; 
    }
    // card 3: Device [USB PnP Sound Device], device 0: USB Audio [USB Audio]
    std::string mic_path = find_alsa_device("USB PnP");
    if (mic_path.empty()) {
        std::cerr << "Error: Could not find microphone device!" << std::endl;
        return -1;
    }
    
    // INIT Vosk Model 
    VoskModel *model = vosk_model_new("../../MicrophoneTest/model/model");
    if (!model) {
        std::cerr << "Error: Could not load Vosk model!" << std::endl;
        return -1;
    }

    // INIT Vosk Recognizer
    VoskRecognizer *recognizer = vosk_recognizer_new(model, 16000.0);

    // INIT Speaker
    UsbSpeaker speaker(speaker_path, myModels, 2 , 0);
    if (!speaker.open()) {
        std::cerr << "Failed to open speaker!" << std::endl;
    }

    // INIT Microphone
    UsbMicrophone mic(mic_path, 16000, 1);
    if (!mic.open()) {
        std::cerr << "Failed to open microphone!" << std::endl;
        return -1;
    }

    std::cout << "\n===========================================" << std::endl;
    std::cout << ">>> voice interaction test <<<" << std::endl;
    std::cout << "test by 'hello', 'status', or 'exit'" << std::endl;
    std::cout << "===========================================\n" << std::endl;

    // receive audio data from microphone and process it with vosk
    // after revognize the text, do some interaction logic and play response with speaker
    // this part should add to init function or a class to manage better
    // change in latest version
    auto audioHandler = [&](const std::vector<short>& data) {
        if (vosk_recognizer_accept_waveform(recognizer, 
                                           (const char*)data.data(), 
                                           data.size() * sizeof(short))) {
            
            std::string result = vosk_recognizer_result(recognizer);
            std::string text = extractText(result);
            
            if (!text.empty() && text != " ") {
                std::cout << "[Recognized]: " << text << std::endl;
                
                // interaction logic
                // GetCommand(text);

                if (text.find("hello") != std::string::npos) {
                    std::cout << ">> go to Hello" << std::endl;
                    speaker.play("what can I help you");
                } 
                else if (text.find("name") != std::string::npos) {
                    std::cout << ">> go to Status" << std::endl;
                    speaker.play("System alert! All systems are running within normal parameters.");
                }
                else if (text.find("stop") != std::string::npos || text.find("exit") != std::string::npos) {
                    std::cout << ">> go to exit" << std::endl;
                    speaker.play("System shutting down. Goodbye.");
                    keepRunning = false; 
                }
            }
        }
    };

    //start microphone capture and processing
    mic.start(audioHandler);

    // use a simple loop to keep the main thread alive while the microphone and speaker threads are running
    // in latest version, this part should change to mutex to manage the state of system
    while (keepRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Cleaning up resources..." << std::endl;
    mic.stop();
    speaker.stop();
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);

    return 0;
}