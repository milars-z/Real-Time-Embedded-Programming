#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

struct VoskModel;
struct VoskRecognizer;

class UsbMicrophone;

using TextCallback = std::function<void(std::string)>;

class VoiceProducer {
private:
    std::unique_ptr<UsbMicrophone> mic;
    
    VoskModel* model = nullptr;          
    VoskRecognizer* recognizer = nullptr; 

    TextCallback onTextReady;

public:
    VoiceProducer(const std::string& path, TextCallback callback);
    ~VoiceProducer();

    void start();
    void stop();
};