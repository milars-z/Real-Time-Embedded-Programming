#include "Screen_ui.hpp"
#include <iostream>

struct DPadPayload {
    ScreenUI* ui;          
    std::string signalStr; 
};

ScreenUI::ScreenUI(ThreadSafeQueue<std::string>* outQueue) : signalQueue(outQueue) {
    showHomeScreen(); // 启动时显示主页
}

void ScreenUI::sendSignal(std::string type, std::string data) {
    std::string payload = type + ":" + data;
    signalQueue->push(payload); 
    printf("[ScreenUI] Sent signal: %s\n", payload.c_str());
}

// 初始界面 
void ScreenUI::showHomeScreen() {

    // 关闭视觉界面状态
    is_in_vision_screen = false;

    lv_obj_t* scr = lv_obj_create(NULL);
    
    // Motion 按钮
    lv_obj_t* btn_m = lv_button_create(scr);
    lv_obj_set_size(btn_m, 200, 100);
    lv_obj_align(btn_m, LV_ALIGN_CENTER, -150, 0);
    lv_label_set_text(lv_label_create(btn_m), "Motion");
    lv_obj_add_event_cb(btn_m, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showMotionScreen();
    }, LV_EVENT_CLICKED, this);

    // Vision 按钮
    lv_obj_t* btn_v = lv_button_create(scr);
    lv_obj_set_size(btn_v, 200, 100);
    lv_obj_align(btn_v, LV_ALIGN_CENTER, 150, 0);
    lv_label_set_text(lv_label_create(btn_v), "Vision");
    lv_obj_add_event_cb(btn_v, [](lv_event_t* e){
        auto ui = (ScreenUI*)lv_event_get_user_data(e);
        ui->sendSignal("VISION_STREAM_START");
        ui->showVisionScreen();
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(scr);
}

// Motion 界面 
void ScreenUI::showMotionScreen() {
    lv_obj_t* scr = lv_obj_create(NULL);

    // 返回键
    lv_obj_t* btn_back = lv_button_create(scr);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_back), "Back");
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showHomeScreen();
    }, LV_EVENT_CLICKED, this);

    // Learn 键 (右上)
    lv_obj_t* btn_learn = lv_button_create(scr);
    lv_obj_align(btn_learn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_label_set_text(lv_label_create(btn_learn), "Learn");
    lv_obj_add_event_cb(btn_learn, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("MOTION_LEARN");
    }, LV_EVENT_CLICKED, this);

    // 两个轮盘
    createDPad(scr, -180, "left");
    createDPad(scr, 180, "right");

    // OK 键 (中间)
    lv_obj_t* btn_ok = lv_button_create(scr);
    lv_obj_set_size(btn_ok, 80, 80);
    lv_obj_align(btn_ok, LV_ALIGN_CENTER, 0, 50);
    lv_label_set_text(lv_label_create(btn_ok), "OK");
    lv_obj_add_event_cb(btn_ok, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("MOTION_CONFIRM");
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(scr);
}

// Vision 界面 
void ScreenUI::showVisionScreen() {
    is_in_vision_screen = true; 
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_palette_main(LV_PALETTE_GREY), 0);

    vision_img_obj = lv_image_create(scr);
    lv_obj_set_size(vision_img_obj, 640, 480); // 假设视频大小
    lv_obj_align(vision_img_obj, LV_ALIGN_CENTER, -50, 0); 


    // 初始化描述符（假设 800 宽，根据 SDL 配置可能需要 RGB888）
    vision_img_dsc.header.cf = LV_COLOR_FORMAT_RGB888;
    vision_img_dsc.header.w = 640;
    vision_img_dsc.header.h = 480;
    vision_img_dsc.header.stride = 640 * 3;
    vision_img_dsc.data_size = 640 * 480 * 3;

    // 分配持久化缓冲区
    if(!canvas_buffer) canvas_buffer = (uint8_t*)malloc(640 * 480 * 3);
    vision_img_dsc.data = canvas_buffer;

    lv_screen_load(scr);


    // Learn & Detect 
    lv_obj_t* btn_l = lv_button_create(scr);
    lv_obj_align(btn_l, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_label_set_text(lv_label_create(btn_l), "Learn");
    lv_obj_add_event_cb(btn_l, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("VISION_LEARN");
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_d = lv_button_create(scr);
    lv_obj_align(btn_d, LV_ALIGN_TOP_RIGHT, -10, 70);
    lv_label_set_text(lv_label_create(btn_d), "Detect");
    lv_obj_add_event_cb(btn_d, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("VISION_DETECT");
    }, LV_EVENT_CLICKED, this);

    // Update 
    lv_obj_t* btn_u = lv_button_create(scr);
    lv_obj_align(btn_u, LV_ALIGN_TOP_RIGHT, -10, 130);
    lv_label_set_text(lv_label_create(btn_u), "Update");
    lv_obj_add_event_cb(btn_u, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("VISION_UPDATE");
    }, LV_EVENT_CLICKED, this);

    // 返回
    lv_obj_t* btn_back = lv_button_create(scr);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_back), "Back");
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showHomeScreen();
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(scr);
}

void ScreenUI::createDPad(lv_obj_t* parent, int x_offset, std::string name) {
    
    const char* dirs[] = {"u", "d", "l", "r"};
    int pos_offset[4][2] = {
        {0, -80}, // Up
        {0, 80},  // Down
        {-80, 0}, // Left
        {80, 0}   // Right
    };
    
    for(int i = 0; i < 4; i++) {
        
        // 创建按键位置和大小
        lv_obj_t* btn = lv_button_create(parent);
        lv_obj_set_size(btn, 85, 85); 
        lv_obj_align(btn, LV_ALIGN_CENTER, x_offset + pos_offset[i][0], pos_offset[i][1]);
        
        // 配置按键label
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, dirs[i]);
        lv_obj_center(label);

        
        // 配置信号格式
        std::string full_sig = name + "_" + dirs[i];
        DPadPayload* payload = new DPadPayload{this, full_sig};

        // 绑定信号与按键回调
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            DPadPayload* p = (DPadPayload*)lv_event_get_user_data(e);
            
            // 信号发送
            p->ui->sendSignal("Motion", p->signalStr);
            
        }, LV_EVENT_CLICKED, payload);

        // 删除事件方式泄露
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            DPadPayload* p = (DPadPayload*)lv_event_get_user_data(e);
            delete p; 
        }, LV_EVENT_DELETE, payload);
    }
}

void ScreenUI::createKeyboard(std::string signal_type) {
   
    // 背景模板
    lv_obj_t* mask = lv_obj_create(lv_screen_active());
    lv_obj_set_size(mask, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(mask, LV_OPA_50, 0); 
    lv_obj_set_style_bg_color(mask, lv_palette_main(LV_PALETTE_GREY), 0);

    // 创建容器
    lv_obj_t* cont = lv_obj_create(mask);
    lv_obj_set_size(cont, 600, 80);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW); 
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 创建输入框
    ta_obj = lv_textarea_create(cont);
    lv_obj_set_width(ta_obj, 450);
    lv_textarea_set_placeholder_text(ta_obj, "Input Command...");
    lv_textarea_set_one_line(ta_obj, true);
    lv_obj_add_state(ta_obj, LV_STATE_FOCUSED); 

    // 创建确定按钮
    lv_obj_t* btn_ok = lv_button_create(cont);
    lv_obj_set_size(btn_ok, 80, 45);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, "OK");
    lv_obj_center(lbl_ok);

    // 创建键盘
    kb_obj = lv_keyboard_create(mask);
    lv_obj_set_size(kb_obj, 800, 240); // 占据下半屏
    lv_keyboard_set_textarea(kb_obj, ta_obj);

    // 数据结构体
    struct ConfirmData {
        ScreenUI* ui;
        std::string sig;
        lv_obj_t* mask_to_del;
        lv_obj_t* ta;
    };
    ConfirmData* data = new ConfirmData{this, signal_type, mask, ta_obj};

    // 信号传输回调
    // 发送信号并销毁
    lv_obj_add_event_cb(btn_ok, [](lv_event_t* e) {
        ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
        std::string content = lv_textarea_get_text(d->ta);
        
        d->ui->sendSignal(d->sig, content);
        
        lv_obj_delete(d->mask_to_del);
        delete d; 
    }, LV_EVENT_CLICKED, data);

    // 键盘内的确定
    lv_obj_add_event_cb(kb_obj, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_READY) { 
            lv_obj_t* kb = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* ta = lv_keyboard_get_textarea(kb);
            ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
            
            d->ui->sendSignal(d->sig, lv_textarea_get_text(ta));
            lv_obj_delete(d->mask_to_del);
            delete d;
        } else if(code == LV_EVENT_CANCEL) {
            ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
            lv_obj_delete(d->mask_to_del);
            delete d;
        }
    }, LV_EVENT_ALL, data);
}

void ScreenUI::updateVisionFrame(const cv::Mat& pre_processed_rgb) {
    // 此时 pre_processed_rgb 已经是 640x480 的 RGB 格式
    if (!is_in_vision_screen || pre_processed_rgb.empty() || !vision_img_obj) {
        return;
    }

    // 仅仅做一次内存拷贝，不再进行任何计算操作
    // 如果内存对齐一致，memcpy 是最快的操作
    memcpy(canvas_buffer, pre_processed_rgb.data, 640 * 480 * 3);

    // 刷新显示
    lv_image_set_src(vision_img_obj, &vision_img_dsc);
    lv_obj_invalidate(vision_img_obj);
}