#include "CameraEngine.hpp"

/**
 * CameraEngine - raspi5 CameraEngine
 * 
 * basic reference : berndporr
 * rebuild for raspi5 : Ziyin Zeng
 * License ：GPL
 * Time : 01,30,2026
 * 
 **/

bool CameraEngine::start(int width, int height, int fps) {
    if (active) return true;

    std::string pipeline =        
        "libcamerasrc ! "
        "video/x-raw,format=I420,width=" + std::to_string(width) + 
        ",height=" + std::to_string(height) + 
        ",framerate=" + std::to_string(fps) + "/1 ! "
        "videoconvert ! "
        "video/x-raw,format=BGR ! "
        "appsink drop=true";


    cap.open(pipeline, cv::CAP_GSTREAMER);
    if (!cap.isOpened()) {
        std::cerr << "[Error][CameraEngine] wrong!:can't open GStreamer pipe" << std::endl;
        return false;
    }

    return true;
}

void CameraEngine::start_thread(int core){
    active = true;
    workerThread = std::thread(&CameraEngine::captureLoop, this);
    pinThreadToCore(workerThread, "CamCapThread", core);
}

void CameraEngine::stop_thread() {
    active = false; 
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (cap.isOpened()) {
        cap.release();
    }
}

void CameraEngine::captureLoop() {
    cv::Mat frame;
    while (active) {
        if (!cap.read(frame) || frame.empty()) {
            continue;
        }
        if (callback) {
            callback(frame);
        }
    }
}
