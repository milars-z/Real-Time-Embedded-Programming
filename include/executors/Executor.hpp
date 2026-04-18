#pragma once
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <iostream>

template <typename T>
class BaseExecutor {
protected:
    std::deque<T> taskQueue;
    std::mutex mtx;
    std::condition_variable cv;
    std::thread worker;
    std::atomic<bool> isRunning{false};

    virtual std::string get_module_name() = 0; 

    virtual void onExecute(const T& task) = 0;

    virtual void _stop() = 0;

    virtual void _start(int core) = 0;

    void threadLoop() {
        while (isRunning) {
            T task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !taskQueue.empty() || !isRunning; });
                if (!isRunning && taskQueue.empty()) break;
                task = std::move(taskQueue.front());
                taskQueue.pop_front();
            }
            onExecute(task);
        }
    }

public:
    virtual ~BaseExecutor()= default;

    void start() {
        if (isRunning) return;
        isRunning = true;
        worker = std::thread(&BaseExecutor::threadLoop, this);
    }

    void stop() {
        isRunning = false;
        cv.notify_all();
        if (worker.joinable()) worker.join();
        std::cout << "[End][Executor] Task thread stopped:" << get_module_name() << std::endl;
    }

    void pushTask(T task) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            taskQueue.push_back(std::move(task));
        }
        cv.notify_one();
    }
};