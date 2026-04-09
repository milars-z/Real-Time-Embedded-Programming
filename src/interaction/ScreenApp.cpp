#include "ScreenApp.hpp"

#include "Screen_ui.hpp"
#include "CameraApp.hpp"
#include <iostream>

ScreenProducer::ScreenProducer(std::shared_ptr<CameraExecutor> cam, 
                               UISignalCallback callback) 
    : camera(cam), onSignalReady(callback) {
}

ScreenProducer::~ScreenProducer() {
    stop();
}

void ScreenProducer::start() {
    std::cout << "[Screen] 正在初始化 LVGL 环境..." << std::endl;
    
    lv_init();
    lv_sdl_window_create(800, 480);
    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();

    // 回调函数设计，当检测到按键触发时将信息发送到Brain处理
    ui = std::make_unique<ScreenUI>([this](std::string type, std::string data) {
        if (onSignalReady) {
            onSignalReady(type, data);
        }
    });
}


void ScreenProducer::stop() {
    std::cout << "[Screen] 正在停止 UI..." << std::endl;
}

uint32_t ScreenProducer::update() {

    uint32_t next_ms = lv_timer_handler();

    if (ui && ui->is_in_vision_screen) {
        if (camera) {
            cv::Mat frame = camera->getLatestFrame(); 
            if (!frame.empty()) {
                ui->updateVisionFrame(frame);
            }
        }
    }
    return next_ms;
}