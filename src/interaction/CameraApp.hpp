#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp> 
#include "Executor.hpp"       

class CameraHandle;

class CameraExecutor : public BaseExecutor<std::string> {
private:
    std::unique_ptr<CameraHandle> cam;
    std::mutex frameMtx;

public:
    CameraExecutor();
    ~CameraExecutor(); 

    void onExecute(const std::string& task) override;

    cv::Mat getLatestFrame();
    
    void findObject(const std::string& name);
    void learnObject(const std::string& name);

    std::string action = "None";
    std::string target = "None";
};