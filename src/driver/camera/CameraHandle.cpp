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
    
    // 线程绑定单核
    cv::setNumThreads(1);

    // 设置相机回调
    cam.onFrame([this](const cv::Mat& img) {

        // 节省内存，当没有指令且不显示画面时直接丢弃图像数据
        if (!is_display_enabled && state == CamState::IDLE) {
            return; 
        }

        if (img.empty()) 
            return;

        if (is_display_enabled) {
            std::lock_guard<std::mutex> lock(display_mtx);
            img.copyTo(display_frame);
        }

        // 如果有任务，则将MATpop进queue
        if (state.load() != CamState::IDLE) {
            camera_queue.push(img.clone());
        }
    });
};

//解构的时候停止相机硬件

CameraHandle::~CameraHandle() = default;

// 内部线程启动
// 优先开启底层线程
void CameraHandle::start_thread(int core){
    
    cam.start_thread(core);

    running = true;
    cameraThread = std::thread(&CameraHandle::cameraWorker, this);
    pinThreadToCore(cameraThread, "CamWorkThread", core);
}

// 内部线程关闭
//最后关闭底层线程
void CameraHandle::stop_thread(){
    
    running = false;
    camera_queue.stop();
    if (cameraThread.joinable()) cameraThread.join();

    cam.stop_thread();
    
}


// Cam硬件启动
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

// 外部调用函数
// 更新背景
void CameraHandle::Update_bg() {
    startTask(CamState::UPDATING_BG);
};

// 外部调用函数
// 学习物体
void CameraHandle::Learn_obj(const std::string name) {
    target_name = name;
    startTask(CamState::LEARNING);
};

// 外部调用函数
// 查找物体
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
    std::cout << "[CameraHandle]Task Started: Sampling images..." << std::endl;
};


// Cam图像处理线程
// 保证稳定性，取5张只用最后一张进行处理
// 在推流状态下取到合适的mat再根据Task进行处理
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

// 实际执行的函数，根据当前状态进行不同的处理
// 包括更新背景，obj学习，obj检测
// 内部使用high_resolution_clock计算检测时间
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
            _taskdescribe.Name = "NoBackground"; // 通过name来判断是否学习失败
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
        _taskdescribe.TaskType = "Detecte";
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


// 在推流后绘制图用
// 显示检测结果，框，检测时间
// 在main循环中调用update() -> getLatestFrame() -> getProcessedFrame()
// 返回mat
cv::Mat CameraHandle::getProcessedFrame() {
    cv::Mat canvas;
    std::vector<DetectedObject> objs;
    double ms;
    int match_idx = last_found_index.load();

    // 获取画图数据
    {
        std::lock_guard<std::mutex> lock(display_mtx);
        if (display_frame.empty()) return cv::Mat();
        canvas = display_frame.clone();
    }

    // 如果有检测结果
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



