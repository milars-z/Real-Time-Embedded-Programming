#include "CameraApp.hpp"
#include "CameraHandle.hpp"
#include "Config.hpp"
#include <iostream>


CameraExecutor::CameraExecutor() {

    cam = std::make_unique<CameraHandle>(
        Config::Camera::CAMERA_MODEL,
        Config::Camera::CAMERA_FEATURE
    );
    if (!cam->open()) {
        std::cerr << "[CameraExecutor] Hardware initialization failed!" << std::endl;
    }
}

CameraExecutor::~CameraExecutor() {
    std::cout << "[CameraExecutor] destructor end" << std::endl;
}

void CameraExecutor::pinThread(int num){
    pinThreadToCore(this->worker, "CameraTask", num);
}

std::string CameraExecutor::get_module_name(){
    return "Camera";
}

// 阻塞退出
void CameraExecutor::_stop(){
    if(cam){
        cam->stop_thread();
        std::cout << "[CameraApp] Worker thread exited..." << std::endl;
    }
}

void CameraExecutor::_start(int core){
    cam->start_thread(core);
    std::cout << "[CameraApp] Internal thread started, pinned to core:" << core << std::endl;
}

void CameraExecutor::onExecute(const std::string& task) {

    std::cout << "[CameraApp] Executing task: " << task << std::endl;

    
    // 任务由brain下发，现在已经确定的任务有以下几种
    // FINDOBJ:apple
    // LEARNOBJ:apple
    // UPDATEBG
    // 首先经过分词提取意图

    CameraCommand cmd; 
    cmd = analyzecommand(task);

    if (cmd.command == "FINDOBJ"){
        cam->Find_obj(cmd.obj);
        auto pos_res = cam->getObjectPosition();
        std::cout << "[CameraApp] Target position: x=" << pos_res.x << " y=" << pos_res.y << std::endl;
    }else if(cmd.command == "LEARNOBJ"){
        cam->Learn_obj(cmd.obj);
        std::cout << "[CameraApp] Object features saved: " << cmd.obj << std::endl;
    }else if(cmd.command == "UPDATEBG"){
        cam->Update_bg();
        std::cout << "[CameraApp] Background updated " << std::endl;
    }
    
}

// 外部调用
cv::Mat CameraExecutor::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frameMtx);
    return cam->getProcessedFrame();
}

CameraCommand CameraExecutor::analyzecommand(const std::string& text){

    CameraCommand cmd;

    if (text == "UPDATEBG") {
        cmd.command = "UPDATEBG";
        cmd.obj = "";
        return cmd;
    }

    const std::string find_prefix = "FINDOBJ:";
    if (text.rfind(find_prefix, 0) == 0) {
        cmd.command = "FINDOBJ";
        cmd.obj = text.substr(find_prefix.size());
        return cmd;
    }

    const std::string learn_prefix = "LEARNOBJ:";
    if (text.rfind(learn_prefix, 0) == 0) {
        cmd.command = "LEARNOBJ";
        cmd.obj = text.substr(learn_prefix.size());
        return cmd;
    }

    return cmd;
}