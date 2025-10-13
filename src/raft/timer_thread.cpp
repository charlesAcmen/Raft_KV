#include "raft/timer_thread.h"
#include <spdlog/spdlog.h>

namespace raft {
    ThreadTimer::ThreadTimer(std::function<void()> cb)
        : running_(false), callback_(std::move(cb)) {
        // The callback `cb` will be invoked when the timer fires.
    }
    ThreadTimer::~ThreadTimer() {
        Stop();
        if (thread_.joinable()) thread_.join();
    }

    void ThreadTimer::Reset(std::chrono::milliseconds duration) {
        spdlog::info("[ThreadTimer] Resetting timer to {} ms", duration.count());
        Stop(); // cancel any running timer

        {
            std::lock_guard<std::mutex> lock(mu_);
            running_ = true;
        }
        thread_ = std::thread([this, duration]() {
            std::unique_lock<std::mutex> lock(mu_);
            // wait_for returns true if Stop() sets running_ = false and notifies
            bool stoppedEarly = cv_.wait_for(lock, duration, [this]() { return !running_; });

            if (!stoppedEarly && running_ && callback_) {
                callback_();
            }
        });
    }
    void ThreadTimer::Stop(){
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (!running_) return;
            running_ = false;
        }
        cv_.notify_all();
    }


    //-----------Factory implementation ----------
    std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
        return std::make_unique<ThreadTimer>(cb);
    }
}// namespace raft