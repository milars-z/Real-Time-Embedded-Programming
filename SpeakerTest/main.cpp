#include "SpeakerEngine.hpp"
#include <chrono>

int main() {
    
    UsbSpeaker speaker("hw:2,0", 44100, 2);

    if (!speaker.open()) {
        std::cerr << "Failed to open speaker!" << std::endl;
        return -1;
    }

    std::cout << "--- Speaker Engine Started ---" << std::endl;
    std::cout << "Commands: [h] Say Hello, [s] System Status, [q] Quit" << std::endl;

    char cmd;
    while (std::cin >> cmd && cmd != 'q') {
        if (cmd == 'h') {
            std::cout << "Trigger: Hello" << std::endl;
            speaker.play("Banana! Potatoes");
        } 
        else if (cmd == 's') {
            std::cout << "Trigger: Status" << std::endl;
            speaker.play("System alert! All systems are running within normal parameters.");
        }
    }

    std::cout << "Stopping..." << std::endl;
    speaker.stop();
    return 0;
}