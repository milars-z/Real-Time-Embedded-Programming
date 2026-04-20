#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cstdint>
#include <atomic>

#include "TaskMonitor.hpp"

// --- Forward Declarations ---
class ScreenUI;
class CameraExecutor;

// --- Callback Type Definitions ---
/// @brief Type definition for UI signal callbacks.
using UISignalCallback = std::function<void(std::string, std::string)>;

class ScreenProducer {
private:
    std::unique_ptr<ScreenUI> ui;                ///< Pointer to the UI management instance
    std::shared_ptr<CameraExecutor> camera;      ///< Shared pointer to the camera executor
    std::shared_ptr<TaskMonitor> _taskMonitor;   ///< Shared pointer to the task monitoring system
    
    UISignalCallback onSignalReady;              ///< Callback triggered when a UI signal is ready

    // --- Task Management ---
    std::atomic<int> task_id = 5000;             ///< Unique identifier for the screen producer task
    
    EmptyResult bg;

public:

    ScreenProducer(std::shared_ptr<CameraExecutor> cam, 
                   UISignalCallback callback,
                   std::shared_ptr<TaskMonitor> taskMonitor);
    
    ~ScreenProducer();

    void start(std::atomic<int>& system_state);   
    void stop();    
    
    /**
     * @brief Heartbeat function called by the main loop.
     * @return uint32_t The interval until the next wake-up in milliseconds.
     */
    uint32_t update();

    

};