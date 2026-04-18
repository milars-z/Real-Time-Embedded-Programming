#include <queue>
#include <mutex>
#include <condition_variable>

#include"TaskMessage.hpp"
#include"ThreadSafeQueue.hpp"
#include "TaskMonitor.hpp"


void TaskMonitor::postEvent(TaskEvent event){
        TaskQueue.push(std::move(event));
}

bool TaskMonitor::waitEvent(TaskEvent& event){
        return TaskQueue.pop(event);
}

void TaskMonitor::stop(){
    TaskQueue.stop();
}

void TaskMonitor::reset(){
    TaskQueue.reset();
}

// 后续优化函数
// 暂未使用
TaskEvent TaskMonitor::generate_Event(const std::string& moduleName,
                                      const int& taskId,
                                      const TaskType& taskType,
                                      const TaskStatus& status,
                                      const ResultData& result,
                                      bool issuccessful)
{
    TaskEvent _task;
    _task.taskId = taskId; // 任务Task_id
    _task.moduleName = moduleName; // 模块名称
    _task.taskType = taskType; // 任务类型
    _task.status = status; // 任务状态
    _task.timestamp = std::chrono::steady_clock::now(); // 当前时间
    _task.result = result; // 结果
    _task.issuccessful = issuccessful;  //是否成功

    return _task;

}


