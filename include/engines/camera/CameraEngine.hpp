#ifndef CAMERA_ENGINE_HPP
#define CAMERA_ENGINE_HPP

#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <iostream>

#include "Tools.hpp"

/**
 * CameraEngine - raspi5 CameraEngine
 * 
 * basic reference : berndporr
 * rebuild for raspi5 : Ziyin Zeng
 * License ：MIT
 * Time : 01,30,2026
 * 
 **/

class CameraEngine {
public:

    using FrameCallback = std::function<void(const cv::Mat&)>;

    CameraEngine() : active(false) {}
    ~CameraEngine() = default ;

    bool start(int width = 640, int height = 480, int fps = 30);
    
    void onFrame(FrameCallback cb) {
        callback = cb;
    }

    void stop_thread();

    void start_thread(int core);

private:
    void captureLoop();

    cv::VideoCapture cap;
    std::thread workerThread;
    std::atomic<bool> active; 
    FrameCallback callback = nullptr;
};

#endif