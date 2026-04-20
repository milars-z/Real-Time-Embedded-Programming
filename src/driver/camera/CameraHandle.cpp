#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <opencv2/opencv.hpp>

#include "CameraHandle.hpp"

CameraHandle::CameraHandle(const std::string &model_path, const std::string &feature_path,std::shared_ptr<TaskMonitor> taskMonitor): 
    _detector(model_path), 
    _feat_mgr(feature_path), 
    state(CamState::IDLE), 
    running(false), 
    is_display_enabled(true),
    _taskMonitor(std::move(taskMonitor))    
{
    
    // Bind processing to a single CPU core
    cv::setNumThreads(1);

    // Set the camera callback
    cam.onFrame([this](const cv::Mat& img) {

        // Save memory by discarding image data directly when there is no task and display is disabled
        if (!is_display_enabled && state == CamState::IDLE) {
            return; 
        }

        if (img.empty()) 
            return;

        if (is_display_enabled) {
            std::lock_guard<std::mutex> lock(display_mtx);
            img.copyTo(display_frame);
        }

        // If there is a task, push the Mat into the queue
        if (state.load() != CamState::IDLE) {
            camera_queue.push(img.clone());
        }
    });
};

//Stop the camera hardware during destruction

CameraHandle::~CameraHandle() = default;

// Start internal threads
// Start the low-level thread first
void CameraHandle::start_thread(int core){
    
    cam.start_thread(core);

    running = true;
    cameraThread = std::thread(&CameraHandle::cameraWorker, this);
    pinThreadToCore(cameraThread, "CamWorkThread", core);
}

// Stop internal threads
// Stop the low-level thread last
void CameraHandle::stop_thread(){
    
    running = false;
    camera_queue.stop();
    if (cameraThread.joinable()) cameraThread.join();

    cam.stop_thread();
    
}


// Start the camera hardware
bool CameraHandle::open() {
    std::cout << "[CameraHandle] Opening Camera Hardware..." << std::endl;
    if (cam.start()) {
        std::cout << "[CameraHandle] Camera started successfully." << std::endl;
        return true;
    } else {
        std::cerr << "[CameraHandle] Failed to start camera." << std::endl;
        return false;
    }
}

// Externally callable function
// Update the background
void CameraHandle::Update_bg() {
    startTask(CamState::UPDATING_BG);
};

// Externally callable function
// Learn an object
void CameraHandle::Learn_obj(const std::string name) {
    target_name = name;
    startTask(CamState::LEARNING);
};

// Externally callable function
// Find an object
void CameraHandle::Find_obj(const std::string name) {
    target_name = name;
    startTask(CamState::FINDING);
};


void CameraHandle::startTask(CamState next_state) {
    state = CamState::IDLE;  
    camera_queue.reset();

    {
        std::lock_guard<std::mutex> lock(_result_mtx);
        _latest_objects.clear();
        last_found_index = -1;
    }

    Camera_worker_buffer.clear();
    state = next_state;
    std::cout << "[Info][CameraHandle]Task Started: Sampling images..." << std::endl;
};


// Camera image processing thread
// For stability, capture 5 frames and process only the last one
// Under streaming conditions, obtain a suitable Mat and process it according to the current task
void CameraHandle::cameraWorker() {

    while (running) {
        cv::Mat img;
        if (camera_queue.pop(img)) {
            Camera_worker_buffer.push_back(img);

            
            if (Camera_worker_buffer.size() >= 5) {

                cv::Mat task_img = Camera_worker_buffer.back(); 

                processTask(task_img); 

                Camera_worker_buffer.clear();
                state.store(CamState::IDLE);
            }
        }
    }
};

// Actual execution function that performs different processing based on the current state
// Includes background updating, object learning, and object detection
// Internally uses high_resolution_clock to measure detection time
void CameraHandle::processTask(const cv::Mat& target_img) {
    
    if (target_img.empty()) return; 

    CamState current_job = state.load();

    std::vector<DetectedObject> objs;
    int match_idx = -1; 
    
    {
        std::lock_guard<std::mutex> lock(_result_mtx);
        last_found_index = -1;
    }

#ifdef TESTMODE
    TaskEvent _taskevent;
    _taskevent.moduleName = "Camera";

    CameraResult detect_ans;
    detect_ans.isdetecte = false;
    detect_ans.objectName = "None";
    detect_ans.position_x = -1;
    detect_ans.position_y = -1;

    CameraResult learn_ans;
    learn_ans.isdetecte = false;
    learn_ans.objectName = "None";
    learn_ans.position_x = -1;
    learn_ans.position_y = -1;

#endif
    
    auto start = std::chrono::high_resolution_clock::now();

    if (current_job == CamState::UPDATING_BG) {

#ifdef TESTMODE
        // send message start
        _taskdescribe.Name = "None";
        _taskdescribe.TaskType = "Update";
        _taskevent.taskId = task_id++;
        _taskevent.status = TaskStatus::STARTED;
        _taskevent.result = bg;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskevent.taskType = _taskdescribe;
        
        _taskMonitor->postEvent(_taskevent);
#endif
        // task
        _detector.update_background(target_img);

#ifdef TESTMODE
        // send message finish
        _taskevent.status = TaskStatus::FINISHED;
        _taskevent.issuccessful = true;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskMonitor->postEvent(std::move(_taskevent));
#endif
        
    } 
    else if (current_job == CamState::LEARNING) {

#ifdef TESTMODE
        // send message start
        _taskdescribe.Name = target_name;
        _taskdescribe.TaskType = "Learn";
        _taskevent.taskId = task_id++;
        _taskevent.status = TaskStatus::STARTED;
        _taskevent.result = bg;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskevent.taskType = _taskdescribe;
        _taskMonitor->postEvent(_taskevent);

#endif
        // task
        objs = _detector.detect(target_img);
        if(objs.size() == 0){
#ifdef TESTMODE
            _taskdescribe.Name = "NoBackground"; // Use the name field to determine whether learning failed
            _taskevent.taskType = _taskdescribe;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.issuccessful = false;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(std::move(_taskevent));
#endif
            return;
        }


        if (!objs.empty()) {
            int max_idx = 0; float max_area = 0;
            for(int i=0; i<objs.size(); i++) {
                if(objs[i].score > max_area) { max_area = objs[i].score; max_idx = i; }
            }
            _feat_mgr.save_feature(objs[max_idx], target_name);

#ifdef TESTMODE
            // send message end
            learn_ans.isdetecte = true;
            learn_ans.objectName = target_name;
            _taskevent.result = learn_ans;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.issuccessful = true;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(std::move(_taskevent));
#endif

        }else{
#ifdef TESTMODE
            // send message end
            learn_ans.isdetecte = false;
            learn_ans.objectName = target_name;
            _taskevent.result = learn_ans;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.issuccessful = false;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(std::move(_taskevent));
#endif
        }
    } 
    else if (current_job == CamState::FINDING) {
#ifdef TESTMODE        
        //send message start
        _taskdescribe.Name = target_name;
        _taskdescribe.TaskType = "Detect";
        _taskevent.taskId = task_id++;
        _taskevent.status = TaskStatus::STARTED;
        _taskevent.result = bg;
        _taskevent.timestamp = std::chrono::steady_clock::now();
        _taskevent.taskType = _taskdescribe;
        _taskMonitor->postEvent(_taskevent);
#endif
        // task
        objs = _detector.detect(target_img);
        if(objs.size() == 0){
#ifdef TESTMODE
            _taskdescribe.Name = "NoBackground";
            _taskevent.taskType = _taskdescribe;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskevent.issuccessful = false;
            _taskMonitor->postEvent(std::move(_taskevent));
#endif
            
            return;
        }
        
        match_idx = _feat_mgr.match_object(target_name, objs, 0.2f);


        if (match_idx != -1) {
            
            cv::Rect _box = objs[match_idx].box ;
#ifdef TESTMODE
            // send message
            detect_ans.isdetecte = true;
            detect_ans.objectName = target_name;
            detect_ans.position_x = _box.x + _box.width/2;
            detect_ans.position_y = _box.y + _box.height/2;
            _taskevent.result = detect_ans;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.issuccessful = true;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(std::move(_taskevent));
            // end
#endif

        } else {
#ifdef TESTMODE
            // send message
            detect_ans.isdetecte = false;
            detect_ans.objectName = target_name;
            detect_ans.position_x = -1;
            detect_ans.position_y = -1;
            _taskevent.result = detect_ans;
            _taskevent.status = TaskStatus::FINISHED;
            _taskevent.issuccessful = false;
            _taskevent.timestamp = std::chrono::steady_clock::now();
            _taskMonitor->postEvent(std::move(_taskevent));
            // end
#endif
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::lock_guard<std::mutex> lock(_result_mtx);
    _latest_objects = objs;
    _last_inference_ms = duration;
    last_found_index = match_idx;
    
};


// Used for drawing after streaming
// Display detection results, bounding boxes, and detection time
// Called in the main loop through update() -> getLatestFrame() -> getProcessedFrame()
// Return a mat
cv::Mat CameraHandle::getProcessedFrame() {
    cv::Mat canvas;
    std::vector<DetectedObject> objs;
    double ms;
    int match_idx = last_found_index.load();

    // Get drawing data
    {
        std::lock_guard<std::mutex> lock(display_mtx);
        if (display_frame.empty()) return cv::Mat();
        canvas = display_frame.clone();
    }

    // If there are detection results
    if (match_idx != -1) {

        {
        std::lock_guard<std::mutex> lock(_result_mtx);
        objs = _latest_objects;
        ms = _last_inference_ms;
        }

        for (int i = 0; i < objs.size(); i++) {
            auto& obj = objs[i];
            
            cv::Scalar color = (i == match_idx) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
            int thickness = (i == match_idx) ? 3 : 2;

            cv::rectangle(canvas, obj.box, color, thickness);
            
            std::string label = (i == match_idx) ? "MATCH: " + target_name : "ID: " + std::to_string(obj.id);
            cv::putText(canvas, label, cv::Point(obj.box.x, obj.box.y - 10), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }

        if (ms > 0) {
            std::string time_text = cv::format("Inference: %.2f ms", ms);
            cv::putText(canvas, time_text, cv::Point(20, 30), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 0), 2);
        }
    }

    return canvas;
}



