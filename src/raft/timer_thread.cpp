#include "raft/timer_thread.h"
#include <spdlog/spdlog.h>

namespace raft {
    ThreadTimer::ThreadTimer(std::function<void()> cb)
        : running_(false), callback_(std::move(cb)) {
        // The callback `cb` will be invoked when the timer fires.
    }
    ThreadTimer::~ThreadTimer() {
        Stop();
    }

    void ThreadTimer::Reset(std::chrono::milliseconds duration) {
        Stop(); // cancel any running timer

        running_ = true;
        thread_ = std::thread([this, duration]() {
            std::unique_lock<std::mutex> lock(mu_);
            // parameter:lock, duration, predicate
            // notified by stop(),running is false, return true,wait_for returns true
            if (cv_.wait_for(lock, duration, [this]() { return !running_; })) {
                // stopped early
                return;
            }

            // spdlog::info("[ThreadTimer] Timer expired after {} ms", duration.count());
            // Timer fired normally
            if (running_ && callback_) {
                callback_();
            }
        });
    }
    void ThreadTimer::Stop(){
        {
            std::lock_guard<std::mutex> lock(mu_);
            running_ = false;
        }
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }


    //-----------Factory implementation ----------
    std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
        return std::make_unique<ThreadTimer>(cb);
    }
}// namespace raft