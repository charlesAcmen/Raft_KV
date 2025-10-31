#pragma once
#include <memory>           // std::unique_ptr
#include <chrono>
#include <functional>       // callback
namespace raft {
// Abstract timer interface used by Raft for election timeouts and heartbeats.
class ITimer {
public:
    ITimer(std::function<void()>&);
    virtual ~ITimer() = default;
    // Start or restart timer with new duration
    virtual void Reset(std::chrono::milliseconds duration) = 0;
    // Stop the timer
    virtual void Stop() = 0;
protected:
    std::function<void()> callback_;
    std::chrono::milliseconds duration_{0};
};//class ITimer
// Factory that creates timers. Tests can inject a mock factory that returns
// controllable timers (virtual time) so election determinism and flakiness
// are reduced.
class ITimerFactory {
public:
    virtual ~ITimerFactory() = default;
    // Create a timer that will call `cb` when it fires.
    virtual std::unique_ptr<ITimer> CreateTimer(std::function<void()> cb) = 0;
};//class ITimerFactory
} // namespace raft