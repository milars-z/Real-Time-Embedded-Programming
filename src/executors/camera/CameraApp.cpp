#include "CameraApp.hpp"
#include "CameraHandle.hpp"
#include "Config.hpp"
#include "SystemCode.hpp"
#include <iostream>


CameraExecutor::CameraExecutor(std::atomic<int>& system_state, std::shared_ptr<TaskMonitor> taskMonitor)
:_taskMonitor(taskMonitor)
{

    cam = std::make_unique<CameraHandle>(
        Config::Camera::CAMERA_MODEL,
        Config::Camera::CAMERA_FEATURE,
        _taskMonitor
    );
    if (!cam->open()) {
        std::cerr << "[Error][CameraApp] Hardware initialization failed!" << std::endl;
        system_state |= ERR_CAMERA_INIT;
    }
}

CameraExecutor::~CameraExecutor() {
    std::cout << "[End][CameraApp] destructor end" << std::endl;
}

// 线程绑定
void CameraExecutor::pinThread(int num){
    pinThreadToCore(this->worker, "CameraTask", num);
}

std::string CameraExecutor::get_module_name(){
    return "Camera";
}

// 内部线程退出
void CameraExecutor::_stop(){
    if(cam){
        cam->stop_thread();
        std::cout << "[End][CameraApp] Worker thread exited..." << std::endl;
    }
}

// 内部线程启动
void CameraExecutor::_start(int core){
    if (!cam) return;
    cam->start_thread(core);
    std::cout << "[Init][CameraApp] Internal thread started, pinned to core:" << core << std::endl;
}

// Camera任务执行，线程函数
void CameraExecutor::onExecute(const std::string& task) {

    std::cout << "[Info][CameraApp] Executing task: " << task << std::endl;

    CameraCommand cmd; 
    cmd = analyzecommand(task);

    if (cmd.command == "FINDOBJ"){
        cam->Find_obj(cmd.obj);
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

// Camera任务解析
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