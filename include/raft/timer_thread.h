#pragma once
#include "timer.h"
#include <thread>
#include <atomic>
#include <condition_variable>

namespace raft {

class ThreadTimer : public ITimer {
    public:
        explicit ThreadTimer(std::function<void()>);
        virtual ~ThreadTimer() override;

        void Reset(std::chrono::milliseconds duration) override;

        void Stop() override;
    private:
        void workerLoop();

        // synchronization
        std::mutex mu_;
        std::condition_variable cv_;
        std::atomic<uint64_t> generation_;

        // state (protected by mu_)
        bool running_{false};
        bool stopped_{false};
        std::chrono::milliseconds duration_{0};
        
        std::function<void()> callback_;
        std::thread worker_;
};

// Timer factory: creates real timers that use threads.
class ThreadTimerFactory : public ITimerFactory {
public:
    explicit ThreadTimerFactory() = default;
    std::unique_ptr<ITimer> CreateTimer(std::function<void()> cb) override;
};

} // namespace raft
