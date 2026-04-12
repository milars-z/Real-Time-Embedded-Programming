#ifndef SCREEN_UI_HPP
#define SCREEN_UI_HPP

#include "lvgl/lvgl.h"
#include "ThreadSafeQueue.hpp"
#include <cstdint> 
#include <string>
#include <opencv2/opencv.hpp>
#include <atomic>


class ScreenUI {
public:
    
    // ScreenUI(ThreadSafeQueue<std::string>* outQueue);
    
    using UIEventCallback = std::function<void(std::string, std::string)>;

    ScreenUI(UIEventCallback callback);
    ~ScreenUI();

    void _stop();

    
    void showHomeScreen();
    void showMotionScreen();
    void showVisionScreen();

    // 推流控制
    void updateVisionFrame(const cv::Mat& pre_processed_rgb);

    std::atomic<bool> is_in_vision_screen = false;   // 状态位

    ThreadSafeQueue<std::string>* signalQueue;

    

private:
    
    // 防止界面切换时内存泄露，创建一个屏幕指针，所有屏幕都用该指针
    lv_obj_t* main_screen = nullptr;

    // LVGL 相关对象
    lv_obj_t* vision_img_obj = nullptr; // 图像显示组件
    lv_image_dsc_t vision_img_dsc;      // LVGL 图像描述符
    // uint8_t* canvas_buffer = nullptr;   // 图像像素缓冲区
    std::vector<uint8_t> canvas_buffer;
    
    

    void sendSignal(std::string type, std::string data = "");
    
    void createKeyboard(std::string signal_type);

    void createDPad(lv_obj_t* parent, int x_offset, std::string name);

    lv_obj_t* kb_obj = nullptr;
    lv_obj_t* ta_obj = nullptr;

    // 图片显示相关
    cv::Mat _ui_ready_frame;       
    std::mutex _ui_frame_mtx;      
    const cv::Size _ui_size = cv::Size(640, 480); 

    UIEventCallback onSignalEvent; 

    // 刷新界面时用
    void prepareMainScreen();

};

#endif