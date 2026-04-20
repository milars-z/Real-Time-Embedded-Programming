#include "CameraApp.hpp"
#include "CameraHandle.hpp"
#include "Config.hpp"
#include "SystemCode.hpp"
#include <iostream>

/**
 * @brief Constructor for CameraExecutor, initializes camera hardware
 * @param system_state Reference to system state atomic variable
 * @param taskMonitor Shared pointer to task monitor
 */
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

/**
 * @brief Destructor for CameraExecutor
 */
CameraExecutor::~CameraExecutor() {
    std::cout << "[End][CameraApp] destructor end" << std::endl;
}

/**
 * @brief Pin the worker thread to a specific CPU core
 * @param num CPU core number to pin to
 */
void CameraExecutor::pinThread(int num){
    pinThreadToCore(this->worker, "CameraTask", num);
}

/**
 * @brief Get the module name
 * @return Module name as string
 */
std::string CameraExecutor::get_module_name(){
    return "Camera";
}

/**
 * @brief Stop the internal worker thread
 */
void CameraExecutor::_stop(){
    if(cam){
        cam->stop_thread();
        std::cout << "[End][CameraApp] Worker thread exited..." << std::endl;
    }
}

/**
 * @brief Start the internal worker thread
 * @param core CPU core number to pin the thread to
 */
void CameraExecutor::_start(int core){
    if (!cam) return;
    cam->start_thread(core);
    std::cout << "[Init][CameraApp] Internal thread started, pinned to core:" << core << std::endl;
}

/**
 * @brief Execute camera task based on command string
 * @param task Task command string to execute
 */
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

/**
 * @brief Get the latest processed frame from camera
 * @return Latest frame as OpenCV Mat
 */
cv::Mat CameraExecutor::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frameMtx);
    return cam->getProcessedFrame();
}

/**
 * @brief Analyze and parse camera command from text
 * @param text Command text to analyze
 * @return Parsed CameraCommand structure
 */
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