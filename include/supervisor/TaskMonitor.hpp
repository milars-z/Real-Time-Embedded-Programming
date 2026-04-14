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

// move处理稍微快点
void postEvent(TaskEvent event){
        TaskQueue.push(std::move(event));
}

bool waitEvent(TaskEvent& event){
        return TaskQueue.pop(event);
}

// 不一定需要，先把模板整体抽象化了
void stop(){
    TaskQueue.stop();
}

void reset(){
    TaskQueue.reset();
}

private:

ThreadSafeQueue<TaskEvent> TaskQueue;

};

