#pragma once

#include <chrono>
#include <variant>
#include <string>

#include "BugCode.hpp"

// 不同模块的结果
struct CameraResult {
    std::string objectName; 
    int position_x;
    int position_y;
    bool isdetecte;
};

struct MotionResult {
    BugCode_M result;
    // --- 
};

struct EmptyResult {

};

struct NluResult {
    bool isdetecte;
    std::string intent = "None";
    std::string name = "None";
};

// 任务描述，用来记录任务干了什么
struct TaskDescribe{
    std::string TaskType;
    std::string Name;
};

using ResultData = std::variant<EmptyResult, CameraResult, MotionResult, NluResult>;
using TaskType = std::variant<TaskDescribe>;


// 任务状态用来匹配获得任务执行所需的时间
enum class TaskStatus { STARTED, FINISHED };

// 队列中的每一条消息
struct TaskEvent {
    int taskId;   // Motion 0000-999; Camera 1000-1999; Speaker 2000-2999; Nlu 3000-3999
    std::string moduleName;  // Motion;Camera;Speaker;Nlu
    TaskType taskType;  // Motion-DoMotion-MotionSet-GetObj; Camera-Update-Detecte-Learn; Nlu analyze
    TaskStatus status;
    std::chrono::steady_clock::time_point timestamp;
    ResultData result;   
    bool issuccessful = false;   
};