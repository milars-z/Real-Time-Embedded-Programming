#include "CameraEngine.hpp"

/**
 * CameraEngine - raspi5 CameraEngine
 *
 * This class manages camera capture on Raspberry Pi 5 using GStreamer pipeline and OpenCV.
 * Provides start, stop, and frame capture functionality with multi-threaded processing.
 *
 * basic reference : berndporr
 * rebuild for raspi5 : Ziyin Zeng
 * License ：GPL
 * Time : 01,30,2026
 *
 **/

/**
 * @brief Start the camera engine and configure the GStreamer pipeline
 * @param width Video frame width in pixels
 * @param height Video frame height in pixels
 * @param fps Frame rate (frames per second)
 * @return true if successfully started, false if already active or pipeline failed to open
 */
bool CameraEngine::start(int width, int height, int fps) {
    // If already active, return true
    if (active) return true;

    // Build GStreamer pipeline string: use libcamerasrc for capture, convert to BGR format, output via appsink
    std::string pipeline =        
        "libcamerasrc ! "
        "video/x-raw,format=I420,width=" + std::to_string(width) + 
        ",height=" + std::to_string(height) + 
        ",framerate=" + std::to_string(fps) + "/1 ! "
        "videoconvert ! "
        "video/x-raw,format=BGR ! "
        "appsink drop=true";


    // Open the GStreamer pipeline using OpenCV
    cap.open(pipeline, cv::CAP_GSTREAMER);
    // Check if the pipeline opened successfully
    if (!cap.isOpened()) {
        std::cerr << "[Error][CameraEngine] wrong!:can't open GStreamer pipe" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Start the capture thread and pin it to the specified CPU core
 * @param core The CPU core number to pin the thread to
 */
void CameraEngine::start_thread(int core){
    // Set the active flag
    active = true;
    // Create the worker thread running the captureLoop method
    workerThread = std::thread(&CameraEngine::captureLoop, this);
    // Pin the thread to the specified core for better performance
    pinThreadToCore(workerThread, "CamCapThread", core);
}

/**
 * @brief Stop the capture thread and release camera resources
 */
void CameraEngine::stop_thread() {
    // Set the active flag to false to signal the thread to stop
    active = false; 
    // Wait for the thread to finish
    if (workerThread.joinable()) {
        workerThread.join();
    }
    // Release the OpenCV video capture object
    if (cap.isOpened()) {
        cap.release();
    }
}

/**
 * @brief Capture loop that continuously reads frames and processes them via callback
 * This method runs in a separate thread, continuously capturing video frames
 */
void CameraEngine::captureLoop() {
    cv::Mat frame;  // Mat object to store the captured frame
    // Main loop, continues while active is true
    while (active) {
        // Read a frame from the camera; if failed or frame is empty, continue to next iteration
        if (!cap.read(frame) || frame.empty()) {
            continue;
        }
        // If a callback is set, call it to process the frame
        if (callback) {
            callback(frame);
        }
    }
}
