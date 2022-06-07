#include "raft.h"
#include <limits>
#include <future>

namespace raft {


RaftTimer::RaftTimer() {
    //std::cout << "New RaftTimer" << std::endl;
    is_running_ = false;
    timeout_ms_ = 0;
    next_timeout_at_ = std::chrono::system_clock::now() + std::chrono::hours(INT_MAX);
}
RaftTimer::~RaftTimer() {
    std::cout << "Delete RaftTimer" << std::endl;
    Deschedule();
}
void RaftTimer::Schedule(uint32_t timeout_ms, std::function<void()> callback) {
    std::cout << "Schedule RaftTimer " << timeout_ms << std::endl;
    timeout_ms_ = timeout_ms;
    callback_ = callback;
    is_running_ = true;
    next_timeout_at_ = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
    thread_ = std::thread([this]() {
        while (is_running_) {
            std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
            if (now >= next_timeout_at_) {
                // 异步执行回调函数，不阻塞计时器下一轮计时
                std::async(std::launch::async, callback_);
                // 重新计时
                next_timeout_at_ = now + std::chrono::milliseconds(timeout_ms_);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
}
void RaftTimer::Deschedule() {
    std::cout << "Deschedule RaftTimer" << std::endl;
    is_running_ = false;
    next_timeout_at_ = std::chrono::system_clock::now() + std::chrono::hours(INT_MAX);
    thread_.join();
}
void RaftTimer::Reset(uint32_t timeout_ms) {
    timeout_ms_ = timeout_ms;
    next_timeout_at_ = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms_);
}


} // namespace raft