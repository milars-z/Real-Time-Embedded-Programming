#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <opencv2/opencv.hpp>

#include "CameraHandle.hpp"

CameraHandle::CameraHandle(const std::string &model_path, const std::string &feature_path): 
    _detector(model_path), 
    _feat_mgr(feature_path), 
    state(CamState::IDLE), 
    running(false), 
    is_display_enabled(true) {
    
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

void CameraHandle::start_thread(int core){
    // 优先开启底层线程
    cam.start_thread(core);

    running = true;
    cameraThread = std::thread(&CameraHandle::cameraWorker, this);
    pinThreadToCore(cameraThread, "CamWorkThread", core);
}


void CameraHandle::stop_thread(){
    
    running = false;
    camera_queue.stop();
    if (cameraThread.joinable()) cameraThread.join();

    //最后关闭底层线程
    cam.stop_thread();
    
}


// Cam硬件启动
bool CameraHandle::open() {
    std::cout << "[CameraEngine] Opening Camera Hardware..." << std::endl;
    if (cam.start()) {
        std::cout << "[CameraEngine] Camera started successfully." << std::endl;
        return true;
    } else {
        std::cerr << "[CameraEngine] Failed to start camera." << std::endl;
        return false;
    }
    cv::namedWindow("Demo", cv::WINDOW_AUTOSIZE);
}

// // Cam硬件关闭
// bool CameraHandle::stop() {
//     std::cout << "[CameraEngine] Closing Camera Hardware..." << std::endl;
//     cam.stop();
//     return true;
// }

    
// 是否持续推流设置
// 后续接外界屏的时候用
// 现在暂时不需要
void CameraHandle::setDisplayEnable(bool enable) {
    is_display_enabled = enable;

};

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

// 获取用于显示的图像
// 后续外接显示模块时用
// 未使用
cv::Mat CameraHandle::getDisplayFrame() {
    std::lock_guard<std::mutex> lock(display_mtx);
    return display_frame.clone();
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
    std::cout << "Task Started: Sampling images..." << std::endl;
};



void CameraHandle::cameraWorker() {

    while (running) {
        cv::Mat img;
        if (camera_queue.pop(img)) {
            Camera_worker_buffer.push_back(img);

            // 保证稳定性，取5张只用最后一张进行处理
            if (Camera_worker_buffer.size() >= 5) {

                cv::Mat task_img = Camera_worker_buffer.back(); 
                          
                // camera内部使用，根据当前状态进行逻辑处理，并绘制框图
                processTask(task_img); 

                // // lvgl侧使用，将最新的结果叠加到UI专用的图上，供UI线程调用
                // prepareUIFrame(task_img); 

                Camera_worker_buffer.clear();
                state.store(CamState::IDLE);
            }
        }
    }
};

// 实际执行的函数，根据当前状态进行不同的处理
// 后续需要设计返回不同的值
// 例如物体的坐标
// 暂时先不返回只print
void CameraHandle::processTask(const cv::Mat& target_img) {
    
    if (target_img.empty()) return; 

    CamState current_job = state.load();

    std::vector<DetectedObject> objs;
    int match_idx = -1; 
    
    {
    // 执行任务之前清空状态
    std::lock_guard<std::mutex> lock(_result_mtx);
    last_found_index = -1;
    }

    // 延迟计算，记录处理时间
    auto start = std::chrono::high_resolution_clock::now();
    
    if (current_job == CamState::UPDATING_BG) {
        _detector.update_background(target_img);
        std::cout << "[Core 2] Background Updated." << std::endl;
    } 
    else if (current_job == CamState::LEARNING) {
        objs = _detector.detect(target_img);
        if(objs.size() == 0){
            std::cout << "[Error][CameraHandle] No Background! " << std::endl;
            return;
        }
        if (!objs.empty()) {
            int max_idx = 0; float max_area = 0;
            for(int i=0; i<objs.size(); i++) {
                if(objs[i].score > max_area) { max_area = objs[i].score; max_idx = i; }
            }
            _feat_mgr.save_feature(objs[max_idx], target_name);
            std::cout << "[Core 2] Learned: " << target_name << std::endl;
        }
    } 
    else if (current_job == CamState::FINDING) {
        objs = _detector.detect(target_img);
        if(objs.size() == 0){
            std::cout << "[Error][CameraHandle] No Background! " << std::endl;
            return;
        }
        match_idx = _feat_mgr.match_object(target_name, objs, 0.2f);


        if (match_idx != -1) {
            std::cout << "[Core 2] Found " << target_name << " at index " << match_idx << std::endl;
            cv::Rect _box = objs[match_idx].box ;
            std::cout << "Bounding Box: x=" << _box.x + _box.width/2 << ", y=" << _box.y + _box.height/2 << std::endl;

        } else {
            std::cout << "[Core 2] " << target_name << " not found." << std::endl;
        }
    }

    // 检测结束，记录时间，并保存结果
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::lock_guard<std::mutex> lock(_result_mtx);
    _latest_objects = objs;
    _last_inference_ms = duration;
    last_found_index = match_idx;
    
};


// 在推流后绘制图用
// 显示检测结果，框，检测时间
// 现已使用，在main循环中调用
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

    // cv::resize(canvas, canvas, _ui_size);
    // cv::cvtColor(canvas, canvas, cv::COLOR_BGR2RGB);

    return canvas;
}

ObjPosition CameraHandle::getObjectPosition() {
    
    std::lock_guard<std::mutex> lock(_result_mtx);

    last_position.x = (last_found_index != -1) ? (_latest_objects[last_found_index].box.x + _latest_objects[last_found_index].box.width / 2) : -1;
    last_position.y = (last_found_index != -1) ? (_latest_objects[last_found_index].box.y + _latest_objects[last_found_index].box.height / 2) : -1;

    return last_position;

}


