#include "CameraApp.hpp"
#include "CameraHandle.hpp"
#include "config_voice.hpp"
#include <iostream>


CameraExecutor::CameraExecutor() {

    cam = std::make_unique<CameraHandle>(
        Config::Camera::CAMERA_MODEL,
        Config::Camera::CAMERA_FEATURE
    );
    
    if (!cam->open()) {
        std::cerr << "[CameraExecutor] 硬件打开失败！" << std::endl;
    }
}

CameraExecutor::~CameraExecutor() = default;

void CameraExecutor::onExecute(const std::string& task) {
    std::cout << "[Camera] 正在执行任务: " << task << std::endl;

    // 任务解析
    // std::string type
    // std:: string data
    // type: MOTION_LEARN
    // MOTION_CONFIRM
    // VISION_LEARN
    // VISION_DETECT
    // VISION_UPDATE
    // DO_MOTION

    // data: obj string
    // motion_name string
    // motion

    action = "None";
    target = "None";


    if (action == "VISION_DETECT") {
        cam->Find_obj(target);
        auto pos_res = cam->getObjectPosition();
        std::cout << "[Camera] 目标位置: x=" << pos_res.x << " y=" << pos_res.y << std::endl;
    } 
    else if (action == "VISION_LEARN") {
        cam->Learn_obj(target);
        std::cout << "[Camera] 物体特征已保存: " << target << std::endl;
    }
    else if (action == "VISION_UPDATE") {
        cam->Update_bg();
    }
}

cv::Mat CameraExecutor::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frameMtx);
    return cam->getProcessedFrame();
}

void CameraExecutor::findObject(const std::string& name) {
    pushTask("FIND:" + name);
}

void CameraExecutor::learnObject(const std::string& name) {
    pushTask("LEARN:" + name);
}