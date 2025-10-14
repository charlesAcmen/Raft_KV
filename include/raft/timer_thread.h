#pragma once
#include "timer.h"
#include <thread>
#include <atomic>
#include <condition_variable>

namespace raft {

class ThreadTimer : public ITimer {
    public:
        explicit ThreadTimer(std::function<void()>);
        ~ThreadTimer() override;

        void Reset(std::chrono::milliseconds duration) override;

        void Stop() override;
    private:
        void workerLoop();

        std::function<void()> callback_;
        bool running_{false};
        bool stopped_{false};
        std::chrono::milliseconds duration_{0};
        std::thread worker_;
        std::mutex mu_;
        std::condition_variable cv_;
};

// Timer factory: creates real timers that use threads.
class ThreadTimerFactory : public ITimerFactory {
public:
    explicit ThreadTimerFactory() = default;
    std::unique_ptr<ITimer> CreateTimer(std::function<void()> cb) override;
};

} // namespace raft
