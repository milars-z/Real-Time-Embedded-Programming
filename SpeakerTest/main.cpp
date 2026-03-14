#include "SpeakerEngine.hpp"
#include <chrono>

int main(int argc, char* argv[]) {
    
    ModelPaths myModels = {
        "../model/vits-piper-en_GB-cori-medium-int8" ,
        "../model/vits-piper-zh_CN-huayan-medium"      
    };

    int langCode = 0;

    if (argc > 1) {
        try {
            langCode = std::stoi(argv[1]); 
        } catch (...) {
            std::cerr << "Invalid input Eng Test now (0)" << std::endl;
            langCode = 0;
        }
    }

    UsbSpeaker speaker("plughw:2,0", myModels, 2 , langCode);

    if (!speaker.open()) {
        std::cerr << "Failed to open speaker!" << std::endl;
        return -1;
    }

    std::cout << "--- Speaker Engine Started ---" << std::endl;
    std::cout << "Commands: [h] English Test, [s] 中文测试, [q] quit" << std::endl;

    char cmd;
    while (std::cin >> cmd && cmd != 'q') {
        if (cmd == 'h') {
            std::cout << "Trigger: Hello" << std::endl;
            speaker.play("English Testing");
        } 
        else if (cmd == 's') {
            std::cout << "Trigger: 中文测试 " << std::endl;
            speaker.play("中文测试中");
        }
    }

    std::cout << "Stopping..." << std::endl;
    speaker.stop();
    return 0;
}