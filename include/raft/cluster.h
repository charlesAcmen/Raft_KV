#pragma once
#include "raft/raft.h"
#include <vector>
#include <memory>               //shared_ptr
#include <condition_variable>
#include <atomic>               //shutdown_requested_ and global_instance_for_signal_

namespace raft {
class Raft; // forward declaration
class Cluster {
public:
    Cluster() = default;
    ~Cluster();

    // Client entry point: submit a command to the cluster
    bool SubmitCommand(const std::string& command);
    
    // create N nodes (IDs 0..N-1) and keep them in nodes_
    void CreateNodes(int n);
    
    void WaitForLeader(int maxAttempts = 50);
    // Start all nodes
    void StartAll();

    // Stop all nodes (orderly)
    void StopAll();

    // Join all node threads
    void JoinAll();

    // Block until SIGINT (Ctrl+C) or StopAll called. Returns when shutting down.
    void WaitForShutdown();

private:
    std::shared_ptr<Raft> GetLeader() const;

    std::vector<std::shared_ptr<Raft>> nodes_;
    std::mutex shutdown_mu_;
    std::condition_variable shutdown_cv_;
    std::atomic<bool> shutdown_requested_{false};

    // static signal handler helper
    static void SignalHandler(int signum);
    // for signal forwarding
    static std::atomic<Cluster*> global_instance_for_signal_; 
};
}// namespace raft
