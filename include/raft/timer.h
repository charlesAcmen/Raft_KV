#pragma once
#include <memory>
#include <chrono>
#include <functional>

namespace raft {


// Abstract timer interface used by Raft for election timeouts and heartbeats.
class ITimer {
    public:
        virtual ~ITimer() = default;

        // Start or restart timer with new duration
        virtual void reset(std::chrono::milliseconds duration) = 0;
        // Start or restart timer with new duration
        virtual void stop() = 0;
};


// Factory that creates timers. Tests can inject a mock factory that returns
// controllable timers (virtual time) so election determinism and flakiness
// are reduced.
class ITimerFactory {
    public:
        virtual ~ITimerFactory() = default;

        // Create a timer that will call `cb` when it fires.
        virtual std::unique_ptr<ITimer> CreateTimer(std::function<void()> cb) = 0;
};


} // namespace raft