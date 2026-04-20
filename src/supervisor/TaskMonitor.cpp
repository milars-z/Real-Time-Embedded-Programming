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

// Subsequent optimization function
// Not yet used
TaskEvent TaskMonitor::generate_Event(const std::string& moduleName,
                                      const int& taskId,
                                      const TaskType& taskType,
                                      const TaskStatus& status,
                                      const ResultData& result,
                                      bool issuccessful)
{
    TaskEvent _task;
    _task.taskId = taskId; // task Task_id
    _task.moduleName = moduleName; // modle name
    _task.taskType = taskType; // Task type
    _task.status = status; // Task status
    _task.timestamp = std::chrono::steady_clock::now(); // current time
    _task.result = result; // result
    _task.issuccessful = issuccessful;  //Whether it is successful

    return _task;

}


