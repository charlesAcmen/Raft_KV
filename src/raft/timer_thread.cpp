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
        ++generation_;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    // spdlog::info("[ThreadTimer] Destroyed and worker joined");
}
void ThreadTimer::Reset(std::chrono::milliseconds duration) {
    // spdlog::info("[ThreadTimer] Resetting to {} ms", duration.count());
    std::lock_guard<std::mutex> lock(mu_);
    duration_ = duration;
    running_ = true; // mark as active
    ++generation_;
    cv_.notify_all();
    // spdlog::info("[ThreadTimer] Timer reset to {} ms", duration.count());
}
void ThreadTimer::Stop(){
    // spdlog::info("[ThreadTimer] Stopping timer");
    {
        std::lock_guard<std::mutex> lock(mu_);
        running_ = false;
        // expiry_ = std::chrono::steady_clock::time_point::min();
    }
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
        if (!running_) {
            // Wait until Reset() starts a new timer or Stop()/destructor ends the loop
            cv_.wait(lock, [this]() { return running_ || stopped_; });
            continue;
        }
        // capture generation and duration under lock (snapshot)
        uint64_t gen = generation_.load();
        auto duration = duration_;
        // compute deadline using steady_clock
        auto deadline = std::chrono::steady_clock::now() + duration;
        // Wait until timeout or early stop
        // wait until is better than wait_for to avoid issues with spurious wakeups虚假唤醒
        bool stoppedEarly = cv_.wait_until(lock, deadline, [this,gen]() { 
            // wake if stopped OR generation changed OR running turned false
            return stopped_ || generation_.load() != gen || !running_;
        });
        // spdlog::info("[ThreadTimer] Woke up from wait, stoppedEarly: {}, stopped_: {}, generation: {}, current generation: {}, running_: {}",
        //     stoppedEarly, stopped_, gen, generation_.load(), running_);
        // If predicate returned true -> either stopped or generation changed or running_ false
        if (stopped_) break;
        // If generation changed or running_ false, skip this round
        if (generation_.load() != gen || !running_) {
            continue;
        }
        // If wait_until returned false, it means timeout reached with generation unchanged
        // (some libraries return false for timeout). We check "now >= deadline" to be safe:
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            // release lock while executing callback
            lock.unlock();
            try {
                if (callback_) callback_();
            } catch (const std::exception& e) {
                spdlog::error("[ThreadTimer] Exception in callback: {}", e.what());
            } catch (...) {
                spdlog::error("[ThreadTimer] Unknown exception in callback");
            }
            lock.lock();
        } else {
            // spurious wake but generation unchanged — continue loop
            continue;
        }
    }
    // spdlog::info("[ThreadTimer] Worker thread exiting");
}
//-----------Factory implementation ----------
std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
    return std::make_unique<ThreadTimer>(cb);
}
}// namespace raft