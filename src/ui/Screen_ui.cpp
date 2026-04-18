#include "Screen_ui.hpp"
#include <iostream>

// 结构体，装自己这个类的指针和btn的string，用来满足lvgl事件回调的要求
struct DPadPayload {
    ScreenUI* ui;  // 向函数内提供内部指针，发送信号用         
    std::string signalStr;  // 信号字符串，由轮盘名称和btn上的方向组合成
};

// 结构体，键盘发数据用
struct ConfirmData {
    ScreenUI* ui;  // 向函数内提供内部指针，发送信号用
    std::string sig_type; // 信号类型，由调用键盘的地方决定
    lv_obj_t* mask_to_del; // 遮罩对象，执行完确认后需要删除
    lv_obj_t* ta; // 输入框
};

ScreenUI::ScreenUI(ScreenUI::UIEventCallback callback)
    : onSignalEvent(callback) {
    showHomeScreen();
}

ScreenUI::~ScreenUI() = default ;

void ScreenUI::_stop(){

    onSignalEvent = nullptr;

    // 键盘遮罩
    lv_obj_t* active_scr = lv_screen_active();
    if (active_scr) {
        lv_obj_clean(active_scr); 
    }

    // 主屏幕
    if (main_screen) {
            lv_obj_delete(main_screen);
            main_screen = nullptr;
        }
}

void ScreenUI::sendSignal(std::string type, std::string data) {
        
    printf("[Info][ScreenUI] Triggered callback: %s:%s\n", type.c_str(), data.c_str());
        if (onSignalEvent) {
            onSignalEvent(type, data);
        }
    }

// 初始界面 
void ScreenUI::showHomeScreen() {

    // 关闭视觉界面状态
    is_in_vision_screen = false;

    prepareMainScreen();

    lv_obj_t* btn_close = lv_button_create(main_screen);
    lv_obj_align(btn_close, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_close), "Close");
    lv_obj_add_event_cb(btn_close, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("STOP_SYSTEM");
    }, LV_EVENT_CLICKED, this);
    
    // Motion 按钮
    lv_obj_t* btn_m = lv_button_create(main_screen);
    lv_obj_set_size(btn_m, 100, 100);
    lv_obj_align(btn_m, LV_ALIGN_CENTER, -150, 0);
    lv_label_set_text(lv_label_create(btn_m), "Motion");
    lv_obj_add_event_cb(btn_m, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showMotionScreen();
    }, LV_EVENT_CLICKED, this);

    // Vision 按钮
    lv_obj_t* btn_v = lv_button_create(main_screen);
    lv_obj_set_size(btn_v, 100, 100);
    lv_obj_align(btn_v, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lv_label_create(btn_v), "Vision");
    lv_obj_add_event_cb(btn_v, [](lv_event_t* e){
        auto ui = (ScreenUI*)lv_event_get_user_data(e);
        ui->showVisionScreen();
    }, LV_EVENT_CLICKED, this);

    // Setting 按钮
    lv_obj_t* btn_s = lv_button_create(main_screen);
    lv_obj_set_size(btn_s, 100, 100);
    lv_obj_align(btn_s, LV_ALIGN_CENTER, 150, 0);
    lv_label_set_text(lv_label_create(btn_s), "Setting");
    lv_obj_add_event_cb(btn_s, [](lv_event_t* e){
        auto ui = (ScreenUI*)lv_event_get_user_data(e);
        ui->showSettingScreen();
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(main_screen);
}

// Motion 界面 
void ScreenUI::showMotionScreen() {

    prepareMainScreen();

    // 返回键
    lv_obj_t* btn_back = lv_button_create(main_screen);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_back), "Back");
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showHomeScreen();
    }, LV_EVENT_CLICKED, this);

    // Learn 键 (右上)
    lv_obj_t* btn_learn = lv_button_create(main_screen);
    lv_obj_align(btn_learn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_label_set_text(lv_label_create(btn_learn), "Learn");
    lv_obj_add_event_cb(btn_learn, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("MOTION_LEARN");
    }, LV_EVENT_CLICKED, this);

    // 两个轮盘
    createDPad(main_screen, -180, "left");
    createDPad(main_screen, 180, "right");

    // OK 键 (中间)
    lv_obj_t* btn_ok = lv_button_create(main_screen);
    lv_obj_set_size(btn_ok, 80, 80);
    lv_obj_align(btn_ok, LV_ALIGN_CENTER, 0, 50);
    lv_label_set_text(lv_label_create(btn_ok), "OK");
    lv_obj_add_event_cb(btn_ok, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("MOTION_CONFIRM");
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_s = lv_button_create(main_screen);
    lv_obj_align(btn_s, LV_ALIGN_TOP_RIGHT, -10, 70);
    lv_label_set_text(lv_label_create(btn_s), "Do");
    lv_obj_add_event_cb(btn_s, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("MOTION_SET");
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_reset = lv_button_create(main_screen);
    lv_obj_align(btn_reset, LV_ALIGN_TOP_RIGHT, -10, 130);
    lv_label_set_text(lv_label_create(btn_reset), "Reset");
    lv_obj_add_event_cb(btn_reset, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("RESET");
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_stop = lv_button_create(main_screen);
    lv_obj_align(btn_stop, LV_ALIGN_TOP_RIGHT, -10, 190);
    lv_label_set_text(lv_label_create(btn_stop), "Stop");
    lv_obj_add_event_cb(btn_stop, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("STOP");
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(main_screen);

    
}

// Vision 界面 
void ScreenUI::showVisionScreen() {

    is_in_vision_screen = true; 

    prepareMainScreen();

    vision_img_obj = lv_image_create(main_screen);
    lv_obj_set_size(vision_img_obj, 640, 480); // 假设视频大小
    lv_obj_align(vision_img_obj, LV_ALIGN_CENTER, -50, 0); 


    // 初始化描述符（假设 800 宽，根据 SDL 配置可能需要 RGB888）
    memset(&vision_img_dsc, 0, sizeof(lv_image_dsc_t));
    vision_img_dsc.header.cf = LV_COLOR_FORMAT_RGB888;
    vision_img_dsc.header.w = 640;
    vision_img_dsc.header.h = 480;
    vision_img_dsc.header.stride = 640 * 3;
    vision_img_dsc.data_size = 640 * 480 * 3;

    // malloc不能用
    // 分配持久化缓冲区
    // if(!canvas_buffer) canvas_buffer = (uint8_t*)malloc(640 * 480 * 3);
    // vision_img_dsc.data = canvas_buffer;

    size_t required_size = 640 * 480 * 3;
    if(canvas_buffer.size() != required_size) {
        canvas_buffer.resize(required_size);
    }

    vision_img_dsc.data = canvas_buffer.data();

    lv_image_set_src(vision_img_obj, &vision_img_dsc);

    // Learn & Detect 
    lv_obj_t* btn_l = lv_button_create(main_screen);
    lv_obj_align(btn_l, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_label_set_text(lv_label_create(btn_l), "Learn");
    lv_obj_add_event_cb(btn_l, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("VISION_LEARN");
    }, LV_EVENT_CLICKED, this);

    lv_obj_t* btn_d = lv_button_create(main_screen);
    lv_obj_align(btn_d, LV_ALIGN_TOP_RIGHT, -10, 70);
    lv_label_set_text(lv_label_create(btn_d), "Detect");
    lv_obj_add_event_cb(btn_d, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("VISION_DETECT");
    }, LV_EVENT_CLICKED, this);

    // Update 
    lv_obj_t* btn_u = lv_button_create(main_screen);
    lv_obj_align(btn_u, LV_ALIGN_TOP_RIGHT, -10, 130);
    lv_label_set_text(lv_label_create(btn_u), "Update");
    lv_obj_add_event_cb(btn_u, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("VISION_UPDATE");
    }, LV_EVENT_CLICKED, this);

    // Update 
    lv_obj_t* btn_reset = lv_button_create(main_screen);
    lv_obj_align(btn_reset, LV_ALIGN_TOP_RIGHT, -10, 190);
    lv_label_set_text(lv_label_create(btn_reset), "Reset");
    lv_obj_add_event_cb(btn_reset, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->sendSignal("RESET");
    }, LV_EVENT_CLICKED, this);

    // 返回
    lv_obj_t* btn_back = lv_button_create(main_screen);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_back), "Back");
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showHomeScreen();
    }, LV_EVENT_CLICKED, this);

    lv_screen_load(main_screen);
}

void ScreenUI::showSettingScreen(){

    is_in_vision_screen = false;

    config_var _var = screen_get_var();

    prepareMainScreen();

    // 返回
    lv_obj_t* btn_back = lv_button_create(main_screen);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lv_label_create(btn_back), "Back");
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->showHomeScreen();
    }, LV_EVENT_CLICKED, this);

    // LABEL
    lv_obj_t* label_host = lv_label_create(main_screen);
    std::string host_str = "Host Name: " + _var.host; 
    lv_label_set_text(label_host, host_str.c_str());
    lv_obj_align(label_host, LV_ALIGN_TOP_LEFT, 50, 100);

    // EDIT
    lv_obj_t* btn_edit_host = lv_button_create(main_screen);
    lv_obj_set_size(btn_edit_host, 80, 40);
    lv_obj_align(btn_edit_host, LV_ALIGN_TOP_LEFT, 350, 90);
    lv_label_set_text(lv_label_create(btn_edit_host), "Edit");
    
    //SEND
    lv_obj_add_event_cb(btn_edit_host, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("SET_HOST_NAME");
    }, LV_EVENT_CLICKED, this);

    // LABEL
    lv_obj_t* label_robot = lv_label_create(main_screen);
    std::string robot_str = "Robot Name: " + _var.robot; 
    lv_label_set_text(label_robot, robot_str.c_str());
    lv_obj_align(label_robot, LV_ALIGN_TOP_LEFT, 50, 170);

    // EDIT
    lv_obj_t* btn_edit_robot = lv_button_create(main_screen);
    lv_obj_set_size(btn_edit_robot, 80, 40);
    lv_obj_align(btn_edit_robot, LV_ALIGN_TOP_LEFT, 350, 160);
    lv_label_set_text(lv_label_create(btn_edit_robot), "Edit");
    
    // SEND
    lv_obj_add_event_cb(btn_edit_robot, [](lv_event_t* e){
        ((ScreenUI*)lv_event_get_user_data(e))->createKeyboard("SET_ROBOT_NAME");
    }, LV_EVENT_CLICKED, this);



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

        
        // 配置需要传入的结构体信息
        std::string full_sig = name + "_" + dirs[i];
        DPadPayload* payload = new DPadPayload{this, full_sig};

        // 绑定信号与按键回调
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            DPadPayload* p = (DPadPayload*)lv_event_get_user_data(e);
            
            // 信号发送
            p->ui->sendSignal("DO_MOTION", p->signalStr);
            
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
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 50);  // 修改了位置
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
    lv_obj_align(kb_obj, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_keyboard_set_textarea(kb_obj, ta_obj);


    ConfirmData* data = new ConfirmData{this, signal_type, mask, ta_obj};

    // 确认键确认
    lv_obj_add_event_cb(btn_ok, [](lv_event_t* e) {
        ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
        std::string content = lv_textarea_get_text(d->ta);
        
        d->ui->sendSignal(d->sig_type, content);

        lv_obj_delete(d->mask_to_del);

        if (d->sig_type == "SET_HOST_NAME" || d->sig_type == "SET_ROBOT_NAME") {
            d->ui->showHomeScreen(); 
        }
        
        delete d; 
    }, LV_EVENT_CLICKED, data);

    // 键盘内的确定与取消事件
    lv_obj_add_event_cb(kb_obj, [](lv_event_t* e) {
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_READY) { 
            lv_obj_t* kb = (lv_obj_t*)lv_event_get_target(e);
            lv_obj_t* ta = lv_keyboard_get_textarea(kb);
            ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
            
            d->ui->sendSignal(d->sig_type, lv_textarea_get_text(ta));
            lv_obj_delete(d->mask_to_del);

            if (d->sig_type == "SET_HOST_NAME" || d->sig_type == "SET_ROBOT_NAME") {
                d->ui->showHomeScreen(); 
            }
            
            delete d;
        } else if(code == LV_EVENT_CANCEL) {
            ConfirmData* d = (ConfirmData*)lv_event_get_user_data(e);
            lv_obj_delete(d->mask_to_del);
            delete d;
        }
    }, LV_EVENT_ALL, data);
}

void ScreenUI::updateVisionFrame(const cv::Mat& frame) {
    if (!is_in_vision_screen || frame.empty() || !vision_img_obj) return;

    // memcpy(canvas_buffer, frame.data, 640 * 480 * 3);

    memcpy(canvas_buffer.data(), frame.data, canvas_buffer.size());

    // 刷新逻辑
    lv_image_set_src(vision_img_obj, &vision_img_dsc);
    lv_obj_invalidate(vision_img_obj);
}

void ScreenUI::prepareMainScreen() {
    if (!main_screen) {
        main_screen = lv_obj_create(NULL);
    } else {
        lv_obj_clean(main_screen);
    }

    lv_obj_set_style_bg_color(main_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(main_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(main_screen, 0, 0);
    lv_obj_set_style_radius(main_screen, 0, 0);
}