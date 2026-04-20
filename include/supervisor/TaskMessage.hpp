#pragma once

#include <chrono>
#include <variant>
#include <string>

#include "BugCode.hpp"

// The results of different modules
struct CameraResult {
    std::string objectName; 
    int position_x;
    int position_y;
    bool isdetecte;
};

struct MotionResult {
    BugCode_M result;
    std::string name;
};

struct EmptyResult {

};

struct NluResult {
    bool isdetecte;
    std::string intent = "None";
    std::string name = "None";
};

/**
 * @brief Task description structure used to record task activities.
 */
struct TaskDescribe{
    std::string TaskType;
    std::string Name;
};

using ResultData = std::variant<EmptyResult, CameraResult, MotionResult, NluResult>;
using TaskType = std::variant<TaskDescribe>;


/**
 * @brief Task status used for tracking and calculating execution duration.
 */
enum class TaskStatus { STARTED, FINISHED };


/**
 * @brief Represents an individual message/event within the task queue.
 */
struct TaskEvent {
    int taskId;   // Motion 0000-999; Camera 1000-1999; Speaker 2000-2999; Nlu 3000-3999
    std::string moduleName;  // Motion;Camera;Speaker;Nlu
    TaskType taskType;  // Motion-DoMotion-MotionSet-GetObj; Camera-Update-Detecte-Learn; Nlu analyze
    TaskStatus status;
    std::chrono::steady_clock::time_point timestamp;
    ResultData result;   
    bool issuccessful = false;   
};