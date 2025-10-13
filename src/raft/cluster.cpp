// cluster.cpp
#include "raft/cluster.h"
#include "raft/raft.h"
#include <spdlog/spdlog.h>
#include <thread>

namespace raft {
std::atomic<Cluster*> Cluster::global_instance_for_signal_{nullptr};

raft::Cluster::~Cluster() {
    // Destructor ensures nodes are stopped and joined.
    StopAll();
    JoinAll();
}

void raft::Cluster::CreateNodes(int n) {
    nodes_.clear();
    for (int i = 0; i < n; ++i) {
        nodes_.push_back(std::make_unique<Raft>(i));
    }
}

void raft::Cluster::StartAll() {
    for (auto &n : nodes_) n->Start();
}

void raft::Cluster::StopAll() {
    for (auto &n : nodes_) n->Stop();
}

void raft::Cluster::JoinAll() {
    for (auto &n : nodes_) n->Join();
}

void raft::Cluster::SignalHandler(int signum) {
    spdlog::info("[Cluster] Caught signal {}.", signum);
    Cluster* inst = global_instance_for_signal_.load();
    if (inst) {
        inst->shutdown_requested_.store(true);
        inst->shutdown_cv_.notify_one();
    }
}

void raft::Cluster::WaitForShutdown() {
    // Register static pointer for signal forwarding.
    global_instance_for_signal_.store(this);

    // Register signal handler (SIGINT for ctrl-c). POSIX signal handling.
    std::signal(SIGINT, Cluster::SignalHandler);
#ifdef SIGTERM
    std::signal(SIGTERM, Cluster::SignalHandler);
#endif

    spdlog::info("[Cluster] Waiting for shutdown (Ctrl+C to exit) ...");

    std::unique_lock<std::mutex> lk(shutdown_mu_);
    // Wait until either shutdown_requested_ becomes true
    shutdown_cv_.wait(lk, [this](){ return shutdown_requested_.load(); });

    spdlog::info("[Cluster] Shutdown requested, stopping cluster...");
    StopAll();
    JoinAll();
}

}// namespace raft