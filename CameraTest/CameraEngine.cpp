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

    //pi5 pipe initial setting (GStreamer)
    // std::string pipeline = 
    //     "libcamerasrc ! "
    //     "video/x-raw,format=I420,width=640,height=480 ! "
    //     "videoconvert ! "
    //     "video/x-raw,format=BGR ! "
    //     "appsink drop=true";
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
        std::cerr << "[CameraEngine] wrong!:can't open GStreamer pipe" << std::endl;
        return false;
    }

    active = true;
    workerThread = std::thread(&CameraEngine::captureLoop, this);
    
    return true;
}

bool CameraEngine::startFromFile(const std::string& path) {
    if (active) return true;

    cap.open(path);
    if (!cap.isOpened()) {
        std::cerr << "[CameraEngine] wrong!:can't open video file: " << path << std::endl;
        return false;
    }

    active = true;
    workerThread = std::thread(&CameraEngine::captureLoop, this);

    return true;
}

void CameraEngine::captureLoop() {
    cv::Mat frame;
    while (active) {
        if (!cap.read(frame) || frame.empty()) {
            continue;
        }
            // do something
        if (callback) {
            callback(frame);
        }
    }
}

void CameraEngine::stop() {
    active = false; 
    if (workerThread.joinable()) {
        workerThread.join();
    }
    if (cap.isOpened()) {
        cap.release();
    }
}