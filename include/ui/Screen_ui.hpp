#ifndef SCREEN_UI_HPP
#define SCREEN_UI_HPP

#include "lvgl.h"
#include "ThreadSafeQueue.hpp"
#include <cstdint> 
#include <string>
#include <opencv2/opencv.hpp>
#include <atomic>

#include "Tools.hpp"


class ScreenUI {
public:
    
    using UIEventCallback = std::function<void(std::string, std::string)>;

    ScreenUI(UIEventCallback callback);
    ~ScreenUI();

    void _stop();

    void updateVisionFrame(const cv::Mat& pre_processed_rgb);

    std::atomic<bool> is_in_vision_screen = false;   // state

    ThreadSafeQueue<std::string>* signalQueue;

private:

/// @name Screen Navigation
    /// @{
    void showHomeScreen();
    void showMotionScreen();
    void showVisionScreen();
    void showSettingScreen();
    /// @}

    /// @brief Send a UI signal with a specific type and optional data.
    void sendSignal(std::string type, std::string data = "");
    
    void createKeyboard(std::string signal_type);
    void createDPad(lv_obj_t* parent, int x_offset, std::string name);
    

    /// @brief Refresh and prepare the main screen interface.
    void prepareMainScreen();

    /** 
     * @brief Shared screen pointer used to prevent memory leaks during transitions.
     * All screens share this pointer to ensure proper resource management.
     */
    lv_obj_t* main_screen = nullptr;

    // --- LVGL Objects ---
    lv_obj_t* vision_img_obj = nullptr; ///< Image display component
    lv_image_dsc_t vision_img_dsc;      ///< LVGL image descriptor
    std::vector<uint8_t> canvas_buffer; ///< Pixel buffer for the image canvas
    
    lv_obj_t* kb_obj = nullptr;
    lv_obj_t* ta_obj = nullptr;

    //Image display related
    cv::Mat _ui_ready_frame;       
    std::mutex _ui_frame_mtx;      
    const cv::Size _ui_size = cv::Size(640, 480); 

    UIEventCallback onSignalEvent; 

    std::string currentLang = "en";

};

#endif