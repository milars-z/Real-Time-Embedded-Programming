#pragma once //IFDEF ...
#include <queue>
#include <mutex>
#include <condition_variable>

// for all input
template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex mtex;
    std::condition_variable cv;
    bool stop_flag = false;

public:
    // lock -> push data -> notify
    void push(T value) {
        std::unique_lock<std::mutex> lock(mtex);
        queue.push(value);
        cv.notify_one(); 
    }

    // lock ->wait data(unlock) -> get data(lock) -> get data(queue.front)and delete data from queue(queue.pop)(unlock)
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mtex);
        cv.wait(lock, [this] { return !queue.empty() || stop_flag; });

        if (queue.empty() && stop_flag) {
            return false; 
        }

        value = queue.front();
        queue.pop();
        return true;
    }

    // set stop_flag to true and notify
    void stop() {
        std::unique_lock<std::mutex> lock(mtex);
        stop_flag = true;
        cv.notify_all();
    }
    
    // set stop_flag to false and empty the queue for use
    // std::queue<T> empty;
    // std::swap(queue, empty);
    // for clean the queue
    void reset() {
        std::unique_lock<std::mutex> lock(mtex);
        stop_flag = false;
        std::queue<T> empty;
        std::swap(queue, empty);
    }
};