#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

#include"TaskMessage.hpp"
#include"ThreadSafeQueue.hpp"

class TaskMonitor{
public:

    void postEvent(TaskEvent event);

    bool waitEvent(TaskEvent& event);


    void stop();

    void reset();

    TaskEvent generate_Event(const std::string& moduleName,
                             const int& taskId, 
                             const TaskType& taskType,
                             const TaskStatus& status,
                             const ResultData& result,
                             bool issuccessful);

private:

    ThreadSafeQueue<TaskEvent> TaskQueue;

};

