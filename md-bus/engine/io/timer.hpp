#pragma once
#include<atomic>
#include<chrono>
#include<functional>
#include<thread>

namespace md {

using Clock = std::chrono::steady_clock;
using Duration = std::chrono::milliseconds;

class SimpleTimer {
private:
    Duration period_;
    std::function<void()> fn_; // returning void function and no parameters
    std::atomic<bool> running_{false};
    std::thread worker_;

public:
    //initializer : 
    SimpleTimer(Duration period, std::function<void()> fn)
        :period_{period}, fn_{std::move(fn)} {}
    
    void start(){
        bool expected = false;
        if(!running_.compare_exchange_strong(expected, true)){
            return; // already running !
        }

        worker_ = std::thread([this]{
            while(running_.load(std::memory_order_relaxed)){
                auto next = Clock::now() + period_;
                fn_();
                std::this_thread::sleep_until(next);
            }
        });
    }

    void stop() {
        bool expected = true;
        if(!running_.compare_exchange_strong(expected, false)){
            return; // already closed
        }
        if(worker_.joinable()){
            worker_.join();
        }
    }

    ~SimpleTimer() {
        stop();
    }

};

}