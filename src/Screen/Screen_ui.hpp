#ifndef SCREEN_UI_HPP
#define SCREEN_UI_HPP

#include "lvgl.h"
#include "ThreadSafeQueue.hpp"
#include <cstdint> 
#include <string>
#include <opencv2/opencv.hpp>


class ScreenUI {
public:
    
    ScreenUI(ThreadSafeQueue<std::string>* outQueue);

    // ScreenUI();
    
    void showHomeScreen();
    void showMotionScreen();
    void showVisionScreen();

    // 推流控制
    void updateVisionFrame(const cv::Mat& pre_processed_rgb);

    bool is_in_vision_screen = false;   // 状态位

    ThreadSafeQueue<std::string>* signalQueue;

private:
    
    

    // LVGL 相关对象
    lv_obj_t* vision_img_obj = nullptr; // 图像显示组件
    lv_image_dsc_t vision_img_dsc;      // LVGL 图像描述符
    uint8_t* canvas_buffer = nullptr;   // 图像像素缓冲区
    
    

    void sendSignal(std::string type, std::string data = "");
    
    void createKeyboard(std::string signal_type);

    void createDPad(lv_obj_t* parent, int x_offset, std::string name);

    lv_obj_t* kb_obj = nullptr;
    lv_obj_t* ta_obj = nullptr;

    // 图片显示相关
    cv::Mat _ui_ready_frame;       // 预处理好的 RGB 图像
    std::mutex _ui_frame_mtx;      // 保护预处理图像的锁
    const cv::Size _ui_size = cv::Size(640, 480); // 目标 UI 尺寸

};

#endif