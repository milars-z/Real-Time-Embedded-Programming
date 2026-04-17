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

TaskEvent TaskMonitor::generate_Event(const std::string& moduleName,
                                      const int& taskId,
                                      const TaskType& taskType,
                                      const TaskStatus& status,
                                      const ResultData& result,
                                      bool issuccessful)
{
    TaskEvent _task;
    _task.taskId = taskId;
    _task.moduleName = moduleName;
    _task.taskType = taskType;
    _task.status = status;
    _task.timestamp = std::chrono::steady_clock::now();
    _task.result = result;
    _task.issuccessful = issuccessful;

    return _task;

}


