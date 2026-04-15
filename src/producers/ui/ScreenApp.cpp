#include "ScreenApp.hpp"

#include "Screen_ui.hpp"
#include "CameraApp.hpp"
#include <iostream>

ScreenProducer::ScreenProducer(std::shared_ptr<CameraExecutor> cam, 
                               UISignalCallback callback,
                               std::shared_ptr<TaskMonitor> taskMonitor) 
    : camera(cam), onSignalReady(callback), _taskMonitor(taskMonitor) {
}

ScreenProducer::~ScreenProducer() {
    std::cout << "[ScreenProducer] destructor end" << std::endl;
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
#ifdef TESTMODE
            // 只记录单个motion动作的响应时间
            if (type == "DO_MOTION"){
                TaskEvent _taskevent;
                TaskDescribe _taskdescribe;
                _taskevent.moduleName = "Screen-Motion";
                _taskevent.taskId = task_id++;
                _taskdescribe.Name = data;
                _taskdescribe.TaskType = type;
                _taskevent.taskType = _taskdescribe;
                _taskevent.status = TaskStatus::STARTED;
                _taskevent.result = bg;
                _taskevent.timestamp = std::chrono::steady_clock::now();
                _taskMonitor->postEvent(_taskevent);
            }
#endif
            onSignalReady(type, data);
        }
    });
}


void ScreenProducer::stop() {
    // stop();
    std::cout << "[Screen] 正在停止 UI..." << std::endl;
    
    // 清空producer的内容
    lv_deinit();
    // 清空screen_ui的内容
    if(ui){
        ui.reset();
    }
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