// cluster.hpp
#pragma once
#include "raft/raft.h"
#include <vector>
#include <memory>
#include <csignal>
#include <condition_variable>
#include <atomic>

namespace raft {
class Raft; // forward declaration
class Cluster {
public:
    Cluster() = default;
    ~Cluster();

    // create N nodes (IDs 0..N-1) and keep them in nodes_
    void CreateNodes(int n);

    // Start all nodes
    void StartAll();

    // Stop all nodes (orderly)
    void StopAll();

    // Join all node threads
    void JoinAll();

    // Block until SIGINT (Ctrl+C) or StopAll called. Returns when shutting down.
    void WaitForShutdown();

private:
    std::vector<std::unique_ptr<Raft>> nodes_;
    std::mutex shutdown_mu_;
    std::condition_variable shutdown_cv_;
    std::atomic<bool> shutdown_requested_{false};

    // static signal handler helper
    static void SignalHandler(int signum);
    static std::atomic<Cluster*> global_instance_for_signal_; // for signal forwarding
};
}// namespace raft
