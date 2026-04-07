#include "Screen_ui.hpp"
#include "lvgl/lvgl.h"
#include <unistd.h>
#include <iostream>


int main(int argc, char* argv[]) {
    // 初始化 LVGL 和 SDL 设备
    lv_init();

    int32_t hor_res = 800;
    int32_t ver_res = 480;
    // SDL 窗口必须在主线程创建
    lv_display_t * disp = lv_sdl_window_create(hor_res, ver_res);
    lv_indev_t * mouse = lv_sdl_mouse_create();
    lv_indev_t * kb = lv_sdl_keyboard_create();

    ThreadSafeQueue<std::string> signal_queue;
    ScreenUI my_ui(&signal_queue);


    std::cout << "[Screen] UI Loop started..." << std::endl;

    // 4. 主循环：负责 UI 刷新和信号消费
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();

        if (my_ui.is_in_vision_screen) {
            cv::Mat ready_frame;
            // // Mat
            // if (camera_engine.getUIReadyFrame(ready_frame)) {
            //     my_ui.updateVisionFrame(ready_frame);
            // }
        }

        if (sleep_ms < 1) sleep_ms = 1;
        if (sleep_ms > 33) sleep_ms = 33; 
        usleep(sleep_ms * 1000);
    }
        
    return 0;
}