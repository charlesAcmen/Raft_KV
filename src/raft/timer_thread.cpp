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

        {
            std::lock_guard<std::mutex> lock(mu_);
            running_ = true;
        }
        thread_ = std::thread([this, duration]() {
            std::unique_lock<std::mutex> lock(mu_);
            // wait_for returns true if Stop() sets running_ = false and notifies
            bool stoppedEarly = cv_.wait_for(lock, duration, [this]() { return !running_; });

            // take a snapshot of callback before releasing the lock
            auto cb = callback_;
            bool stillRunning = running_;
            lock.unlock();  // release timer mutex before calling callback

            if (!stoppedEarly && stillRunning && cb) {
                cb(); // safe: callback runs outside timer mutex
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
        if (thread_.joinable()) thread_.join();
    }


    //-----------Factory implementation ----------
    std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
        return std::make_unique<ThreadTimer>(cb);
    }
}// namespace raft