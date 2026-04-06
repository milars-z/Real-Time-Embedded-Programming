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
#include "VisonTools.hpp"


enum class CamState {
    IDLE,       // 闲置状态，不处理图像，后续追加屏幕再进行修改
    LEARNING,   // 学习/保存特征状态
    FINDING,    // 查找/匹配特征状态
    UPDATING_BG // 更新背景状态
};

struct ObjPosition {
    int x = -1;
    int y = -1;
};

class CameraHandle {
public:

    // 构造
    CameraHandle(const std::string& model_path, const std::string& feature_path);
    
    // 解构
    ~CameraHandle();

    //背景更新
    void Update_bg();

    // obj学习
    void Learn_obj(const std::string name = "obj_1");

    // obj查找
    void Find_obj(const std::string name = "obj_1");

    // 推流开关
    void setDisplayEnable(bool enable);

    // 获取推流，外置显示用
    cv::Mat getDisplayFrame();

    bool open();

    bool stop();

    cv::Mat getProcessedFrame();

    // 获取物体位置
    ObjPosition getObjectPosition();    


private:

    // 根据新的状态启动检测学习相关任务
    void startTask(CamState next_state);

    // 主线程
    void cameraWorker();

    // 实际任务处理，根据最后一帧MAT进行处理
    // 后续会更改返回值
    void processTask(const cv::Mat& target_img);

private:
    
    // 外界调用路径相关
    AttentionDetector _detector;
    FeatureManager    _feat_mgr;

    // Cam硬件驱动
    CameraEngine      cam;

    // CameraHandle线程
    std::thread       cameraThread;

    // CameraHandle队列
    ThreadSafeQueue<cv::Mat> camera_queue;
    
    // CameraHandle构造函数用
    // 构造是否开始
    std::atomic<bool> running;

    // 状态设置
    std::atomic<CamState> state;

    // 是否持续推流
    std::atomic<bool> is_display_enabled;

    // 检测/学习目标名字
    std::string target_name;

    // 持续推流相关
    // 后续使用
    std::mutex display_mtx;
    cv::Mat display_frame;

    // CameraWorker的buffer
    // 用来放MAT图像，当图像大于5张时进行处理
    // 防止抖动
    std::vector<cv::Mat> Camera_worker_buffer;

    // 结果显示相关
    std::vector<DetectedObject> _latest_objects; 
    double _last_inference_ms = 0;              
    std::mutex _result_mtx;     
    
    std::atomic<int> last_found_index;

    ObjPosition last_position;
};

#endif // CAMERA_HANDLE_HPP
