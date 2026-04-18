#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <atomic>
#include <thread>

#include "TaskMonitor.hpp"
#include "ThreadSafeQueue.hpp"

struct VoskModel;
struct VoskRecognizer;

class UsbMicrophone;

using TextCallback = std::function<void(std::string)>;

class VoiceProducer {
private:
    std::unique_ptr<UsbMicrophone> mic;

    std::shared_ptr<TaskMonitor> _taskMonitor;
    
    VoskModel* model = nullptr;          
    VoskRecognizer* recognizer = nullptr; 

    TextCallback onTextReady;

    std::atomic<int> task_id = 4000;
    
    EmptyResult bg;

    void voskWorker();

    ThreadSafeQueue<std::vector<short>> _snddata;

    std::atomic<bool> isrunning;

    std::thread voskThread;

public:
    VoiceProducer(std::atomic<int>& system_state, const std::string& path, TextCallback callback, std::shared_ptr<TaskMonitor> taskMonitor);
    ~VoiceProducer();

    void start();
    void stop();

    void _start(int core);
    void _stop();

    void start_thread(int core);
    void stop_thread();


};