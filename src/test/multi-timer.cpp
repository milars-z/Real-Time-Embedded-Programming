#include <iostream>
#include <thread>
#include <chrono>
#include <atomic> 
#include "KeypressPublisherStdFunc.h"

int main() {
    // timer
    std::atomic<long long> timer{0};
    KeypressPublisherStdFunc keypressPublisher;
    keypressPublisher.registerEventCallback([&]() {
        std::cout << "\n[Event!] User press! now time: " << timer << std::endl;
    });
    keypressPublisher.start();
    std::cout << "main thread start!" << std::endl;
    while (true) {
        timer++; 
        std::this_thread::sleep_for(std::chrono::microseconds(1000)); 
        if (timer % 1000 == 0) {
             std::cout << "main_thread is running ,now time: " << timer << std::endl;
        }
    }
    return 0;
}