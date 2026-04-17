#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

#include"TaskMessage.hpp"
#include"ThreadSafeQueue.hpp"

// monitor负责两项任务，分别是将任务push进队列和pop出队列
// 指针传给每个执行者，用postEvent
// 外部用waitEvent来获取内部Task，然后计算时间并反馈任务

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

