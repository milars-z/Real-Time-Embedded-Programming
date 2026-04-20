#ifndef CAMERA_HANDLE_HPP
#define CAMERA_HANDLE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "ThreadSafeQueue.hpp" 
#include "AttentionDetector.hpp"
#include "FeatureManager.hpp"
#include "CameraEngine.hpp"
#include "Tools.hpp"
#include "TaskMonitor.hpp"


enum class CamState {
    IDLE,       ///< Idle state, no image processing
    LEARNING,   ///< Learning and saving object features
    FINDING,    ///< Finding and matching object features
    UPDATING_BG ///< Updating background model state
};


class CameraHandle {
public:


    CameraHandle(const std::string& model_path, const std::string& feature_path,std::shared_ptr<TaskMonitor> taskMonitor);
    ~CameraHandle();

    // --- External API ---
    /// @brief Update background model.
    void Update_bg();
    /// @brief Learn and find object features.
    void Learn_obj(const std::string name = "obj_1");
    void Find_obj(const std::string name = "obj_1");

    // --- Streaming ---
    /// @brief External LVGL streaming function.
    cv::Mat getProcessedFrame();

    // --- Camera Hardware Control ---
    /// @brief External camera hardware control functions.
    bool open();
    bool stop();

    // --- Thread Control ---
    /// @brief External thread management functions.
    void start_thread(int core);
    void stop_thread();


private:

    /// @brief Start detection or learning tasks based on the new state.
    void startTask(CamState next_state);

    /// @brief Main camera worker thread logic.
    void cameraWorker();

     /// @brief Core task processing based on the latest MAT frame.
    void processTask(const cv::Mat& target_img);

private:
    
    // --- External Modules ---
    AttentionDetector _detector;                ///< Path for external attention detection
    FeatureManager    _feat_mgr;                ///< Feature management instance

    // --- Hardware & Task ---
    CameraEngine      cam;  ///< Camera hardware engine
    std::shared_ptr<TaskMonitor> _taskMonitor;  ///< Task monitoring and management

    // --- Threading & Synchronization ---
    std::thread       cameraThread;             ///< Main processing thread
    ThreadSafeQueue<cv::Mat> camera_queue;      ///< Frame buffer queue
    
    // --- Control Flags (Atomic) ---
    std::atomic<bool> running;            ///< Flag for constructor/system status
    std::atomic<CamState> state;          ///< Current Camera state
    std::atomic<bool> is_display_enabled; ///< Streaming/display toggle

    std::string target_name;     ///< Name of the target for detection/learning

    // --- Display & Buffering ---
    std::mutex display_mtx;
    cv::Mat display_frame;

     /** 
     * @brief Buffer for CameraWorker to reduce jitter (anti-shake).
     * Accumulates MAT images and triggers processing when buffer size exceeds 5.
     */
    std::vector<cv::Mat> Camera_worker_buffer;

    // --- Detection Results ---
    std::vector<DetectedObject> _latest_objects;   ///< List of most recently detected objects
    double _last_inference_ms = 0;                 ///< Duration of the last inference in milliseconds 
    std::mutex _result_mtx;                        ///< Mutex for thread-safe access to results
    
    // --- Detection Tracking ---
    std::atomic<int> last_found_index;

    // --- Task Management ---
    std::atomic<int> task_id = 1000;               ///< Unique identifier for the current task(supervisor)
    TaskDescribe _taskdescribe;                    ///< Metadata describing the current task
    EmptyResult bg;                                ///< Reference for background/empty results

};

#endif // CAMERA_HANDLE_HPP
