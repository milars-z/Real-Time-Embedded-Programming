#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp> 
#include "Executor.hpp"   
#include "TaskMonitor.hpp"    

struct CameraCommand{
    std::string command;
    std::string obj;
};

class CameraHandle;

class CameraExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<CameraHandle> cam;
    std::shared_ptr<TaskMonitor> _taskMonitor;
    std::mutex frameMtx;

private:

    void onExecute(const std::string& task) override;

    std::string get_module_name() override;
    
    void findObject(const std::string& name);
    void learnObject(const std::string& name);

    CameraCommand analyzecommand(const std::string& text);

public:

    CameraExecutor(std::atomic<int>& ststem_state, std::shared_ptr<TaskMonitor> taskMonitor);
    ~CameraExecutor(); 
    
    void _stop() override;

    void _start(int core) override;

    void pinThread(int num);

    cv::Mat getLatestFrame();

};