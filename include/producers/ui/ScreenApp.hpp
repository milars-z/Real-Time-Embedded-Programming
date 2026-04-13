#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cstdint>

// 前置声明
class ScreenUI;
class CameraExecutor;

// 回调类型定义
using UISignalCallback = std::function<void(std::string, std::string)>;

class ScreenProducer {
private:
    std::unique_ptr<ScreenUI> ui;
    std::shared_ptr<CameraExecutor> camera;
    
    UISignalCallback onSignalReady;

public:

    ScreenProducer(std::shared_ptr<CameraExecutor> cam, 
                   UISignalCallback callback);
    
    ~ScreenProducer();

    void start();   
    void stop();    
    
    // 心跳函数：由主循环调用，返回下次唤醒间隔（毫秒）
    uint32_t update();
};