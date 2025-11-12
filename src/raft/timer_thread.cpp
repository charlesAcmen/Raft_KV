#include "raft/timer_thread.h"
#include <spdlog/spdlog.h>
namespace raft {
ThreadTimer::ThreadTimer(std::function<void()>& cb)
    : ITimer(cb),running_(false), stopped_(false),
    generation_(0),worker_(&ThreadTimer::workerLoop, this) {}
ThreadTimer::~ThreadTimer() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        stopped_ = true;
        running_ = false;
        ++generation_;  //bump to wake waiters
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    // spdlog::info("[ThreadTimer] Destroyed and worker joined");
}
void ThreadTimer::Reset(std::chrono::milliseconds duration) {
    // spdlog::info("[ThreadTimer] Resetting to {} ms", duration.count());
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mu_);
    duration_ = duration;
    deadline_ = now + duration;
    running_ = true; // mark as active
    ++generation_;
    // notify worker to re-evaluate (we updated generation_ and deadline_ under lock)
    cv_.notify_all();
    // spdlog::info("[ThreadTimer] Timer reset to {} ms (gen={})", 
    //     duration.count(),generation_.load());
}
void ThreadTimer::Stop(){
    // spdlog::info("[ThreadTimer] Stopping timer");
    std::lock_guard<std::mutex> lock(mu_);
    running_ = false;
    ++generation_;
    cv_.notify_all();
    // spdlog::info("[ThreadTimer] Timer stopped");
}
/**
 * The main loop of the worker thread.
 * 
 * Waits for signals to start or stop the timer.
 * When running_, waits for the specified duration.
 * If not stopped early, executes the callback_.
 */
void ThreadTimer::workerLoop() {
    std::unique_lock<std::mutex> lock(mu_);
    while (!stopped_) {
        // if (!running_) {
        //     // Wait until Reset() starts a new timer or Stop()/destructor ends the loop
        //     cv_.wait(lock, [this]() { return running_ || stopped_; });
        //     continue;
        // }

        // Wait until running_ becomes true or stopped_
        cv_.wait(lock, [this]() { return running_ || stopped_; });
        if (stopped_) break;

        // Now timer is running_. We will snapshot generation and deadline under lock
        uint64_t local_gen = generation_.load();
        std::chrono::steady_clock::time_point local_deadline = deadline_;

        // Loop: wait until deadline or until a Reset/Stop changes generation_/running_/stopped_
        // The predicate returns true if we should wake early 
        // (stop or generation changed or running turned false).
        bool woke_early = cv_.wait_until(lock, local_deadline, [this, local_gen]() {
            return stopped_ || generation_.load() != local_gen || !running_;
        });

        // If stopped -> exit
        if (stopped_) break;

        // If predicate returned true => something changed (generation changed or stop or running false)
        if (woke_early) {
            // generation changed or running turned false; loop back to outer while to re-evaluate
            continue;
        }

        // If we reach here, wait_until returned due to timeout (predicate false) AND generation unchanged
        // So it's a real timer firing.
        // We must call callback WITHOUT holding the lock.
        lock.unlock();
        try {
            if (callback_) {
                // spdlog::info("[ThreadTimer] callback called");
                callback_();
            }
        } catch (const std::exception& e) {
            spdlog::error("[ThreadTimer] Exception in callback: {}", e.what());
        } catch (...) {
            spdlog::error("[ThreadTimer] Unknown exception in callback");
        }
        lock.lock();

        // After firing, we stop the timer and wait for next Reset to re-arm.
        // (This matches typical election-timeout semantics: after firing you wait for Reset to arm it again.)
        running_ = false;
        ++generation_; // bump generation so concurrent Resets are ordered
    }
    // spdlog::info("[ThreadTimer] Worker thread exiting");
}
//-----------Factory implementation ----------
std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
    return std::make_unique<ThreadTimer>(cb);
}
}// namespace raft