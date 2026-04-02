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
    running(true), 
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

    cameraThread = std::thread(&CameraHandle::cameraWorker, this);
    pinThreadToCore(cameraThread, "CamWorkThread", 2);

    
};

CameraHandle::~CameraHandle() {
    stop();
    running = false;
    camera_queue.stop();
    if (cameraThread.joinable()) cameraThread.join();
};


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

// Cam硬件关闭
bool CameraHandle::stop() {
    std::cout << "[CameraEngine] Closing Camera Hardware..." << std::endl;
    cam.stop();
    return true;
}

    
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
cv::Mat CameraHandle::getDisplayFrame() {
    std::lock_guard<std::mutex> lock(display_mtx);
    return display_frame.clone();
};

void CameraHandle::startTask(CamState next_state) {
    camera_queue.reset(); // 清空旧数据
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
                processTask(Camera_worker_buffer.back()); 
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
    last_found_index = -1;

    // 延迟计算，记录处理时间
    auto start = std::chrono::high_resolution_clock::now();
    
    if (current_job == CamState::UPDATING_BG) {
        _detector.update_background(target_img);
        std::cout << "[Core 2] Background Updated." << std::endl;
    } 
    else if (current_job == CamState::LEARNING) {
        objs = _detector.detect(target_img);
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
        match_idx = _feat_mgr.match_object(target_name, objs, 0.2f);

        last_found_index = match_idx;

        if (match_idx != -1) {
            std::cout << "[Core 2] Found " << target_name << " at index " << match_idx << std::endl;
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
};


// 在推流后绘制图用
// 显示检测结果，框，检测时间
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
    {
        std::lock_guard<std::mutex> lock(_result_mtx);
        objs = _latest_objects;
        ms = _last_inference_ms;
    }

    // 绘制框
    for (int i = 0; i < objs.size(); i++) {
        auto& obj = objs[i];
        
        // 如果是匹配到的物体，画红框，否则画绿框
        cv::Scalar color = (i == match_idx) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
        int thickness = (i == match_idx) ? 3 : 2;

        cv::rectangle(canvas, obj.box, color, thickness);
        
        std::string label = (i == match_idx) ? "MATCH: " + target_name : "ID: " + std::to_string(obj.id);
        cv::putText(canvas, label, cv::Point(obj.box.x, obj.box.y - 10), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    }

    // 绘制时间
    if (ms > 0) {
        std::string time_text = cv::format("Inference: %.2f ms", ms);
        cv::putText(canvas, time_text, cv::Point(20, 30), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 0), 2);
    }

    return canvas;
}