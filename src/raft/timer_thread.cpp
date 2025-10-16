    #include "raft/timer_thread.h"
    #include <spdlog/spdlog.h>

    namespace raft {
        ThreadTimer::ThreadTimer(std::function<void()> cb)
            : running_(false), stopped_(false),
            callback_(std::move(cb)), 
            // expiry_(std::chrono::steady_clock::time_point::min()),
            generation_(0),
            worker_(&ThreadTimer::workerLoop, this) {
        }
        ThreadTimer::~ThreadTimer() {
            {
                std::lock_guard<std::mutex> lock(mu_);
                stopped_ = true;
                running_ = false;
                // expiry_ = std::chrono::steady_clock::time_point::min();
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
            // std::chrono::steady_clock::time_point newExpiry;
            // int count = 0;
            // do {
            //     auto now = std::chrono::steady_clock::now();
            //     newExpiry = now + duration_;
            //     count++;
            // } while (newExpiry == expiry_);
            // spdlog::info("[ThreadTimer] Reset looped {} times to get new expiry", count);
            // expiry_ = newExpiry;
            // expiry_ = std::chrono::steady_clock::now() + duration_;
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
                // cv_.wait(lock, [this]() { return running_ || stopped_; });
                // if (stopped_) break;
                // if (!running_) continue;

                // auto myExpiry = expiry_;

                // bool wokeEarly = cv_.wait_until(lock, myExpiry, [this, myExpiry]() {
                //     return stopped_ || !running_ || expiry_ != myExpiry;
                // });
                // auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                //     expiry_ - myExpiry).count();
                // spdlog::info("[ThreadTimer] Woke: wokeEarly={}, stopped_={}, running_={}, myExpiry==expiry_? {}, diff={} ms",
                //          wokeEarly, stopped_, running_, (expiry_ == myExpiry),diff);
                // spdlog::info("[ThreadTimer] Woke: wokeEarly={}, diff={} ms, expired? {}",
                //     wokeEarly,diff,std::chrono::steady_clock::now() >= myExpiry);
                // if (stopped_) break;
                // if (!running_ || expiry_ != myExpiry) continue;

                // lock.unlock();
                // try {
                //     if (callback_) callback_();
                // } catch (const std::exception &e) {
                //     spdlog::error("[ThreadTimer] Exception in callback: {}", e.what());
                // }
                // lock.lock();

                if (!running_) {
                    // Wait until Reset() starts a new timer or Stop()/destructor ends the loop
                    cv_.wait(lock, [this]() { return running_ || stopped_; });
                    continue;
                }
                // spdlog::info("[ThreadTimer] Timer started for {} ms,generation {}", duration_.count(),generation_.load());

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


                // if (!stoppedEarly && running_ && !stopped_ && callback_) {
                //     // if(duration != duration_){
                //     //     spdlog::info("[ThreadTimer] Duration changed during wait, skipping callback");
                //     //     continue; // duration changed during wait, skip callback
                //     // }
                //     // Timer expired normally && 
                //     // still running && 
                //     // not stopped(did not deconstruct) && 
                //     // has callback
                //     // Release lock before running callback to avoid deadlock
                //     lock.unlock();
                //     try {
                //         callback_();//callback_ might call Reset() within,which requires unlock
                //         //otherwise cross lock deadlock
                //     } catch (const std::exception& e) {
                //         spdlog::error("[ThreadTimer] Exception in callback: {}", e.what());
                //     } catch (...) {
                //         spdlog::error("[ThreadTimer] Unknown exception in callback");
                //     }
                //     lock.lock();
                // }
                // ATTENTION: DO NOT SET running_ = false HERE
                // because Reset() might be called within callback_
                // running_ = false;
            }
            // spdlog::info("[ThreadTimer] Worker thread exiting");
        }

        //-----------Factory implementation ----------
        std::unique_ptr<ITimer> ThreadTimerFactory::CreateTimer(std::function<void()> cb){
            return std::make_unique<ThreadTimer>(cb);
        }
    }// namespace raft