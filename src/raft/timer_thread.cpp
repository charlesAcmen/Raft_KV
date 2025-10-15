#include "raft/timer_thread.h"
#include <spdlog/spdlog.h>

namespace raft {
    ThreadTimer::ThreadTimer(std::function<void()> cb)
        : running_(false), stopped_(false),callback_(std::move(cb)), worker_(&ThreadTimer::workerLoop, this) {
    }
    ThreadTimer::~ThreadTimer() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            stopped_ = true;
            running_ = false;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
        // spdlog::info("[ThreadTimer] Destroyed and worker joined");
    }

    void ThreadTimer::Reset(std::chrono::milliseconds duration) {
        // spdlog::info("[ThreadTimer] Resetting to {} ms", duration.count());
        {
            std::lock_guard<std::mutex> lock(mu_);
            duration_ = duration;
            running_ = true; // mark as active
        }
        cv_.notify_all();
        // spdlog::info("[ThreadTimer] Timer reset to {} ms", duration.count());
    }
    void ThreadTimer::Stop(){
        // spdlog::info("[ThreadTimer] Stopping timer");
        {
            std::lock_guard<std::mutex> lock(mu_);
            if(!running_){
                // spdlog::info("[ThreadTimer] Timer already stopped");
                return;
            }
            running_ = false;
        }
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

            std::chrono::milliseconds duration = duration_; // capture duration
            // spdlog::info("[ThreadTimer] Waiting for {} ms", duration.count());

            // Wait until timeout or early stop
            bool stoppedEarly = cv_.wait_for(lock, duration, [this]() { return !running_ || stopped_; });

            if (!stoppedEarly && running_ && !stopped_ && callback_) {
                // Timer expired normally && 
                // still running && 
                // not stopped(did not deconstruct) && 
                // has callback
                // Release lock before running callback to avoid deadlock
                lock.unlock();
                try {
                    callback_();//callback_ might call Reset() within,which requires unlock
                    //otherwise cross lock deadlock
                } catch (const std::exception& e) {
                    spdlog::error("[ThreadTimer] Exception in callback: {}", e.what());
                } catch (...) {
                    spdlog::error("[ThreadTimer] Unknown exception in callback");
                }
                lock.lock();
            }

            // After callback or stop, mark timer as idle
            running_ = false;
        }
        // spdlog::info("[ThreadTimer] Worker thread exiting");
    }

    //-----------Factory implementation ----------
    std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
        return std::make_unique<ThreadTimer>(cb);
    }
}// namespace raft