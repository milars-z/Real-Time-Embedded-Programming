#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp> 
#include "Executor.hpp"       

struct CameraCommand{
    std::string command;
    std::string obj;
};

class CameraHandle;

class CameraExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<CameraHandle> cam;
    std::mutex frameMtx;

public:
    CameraExecutor();
    ~CameraExecutor(); 

    void pinThread(int num);

    void onExecute(const std::string& task) override;

    std::string get_module_name() override;

    void _stop() override;

    void _start(int core) override;

    cv::Mat getLatestFrame();
    
    void findObject(const std::string& name);
    void learnObject(const std::string& name);

    CameraCommand analyzecommand(const std::string& text);

};