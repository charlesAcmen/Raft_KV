#include "raft/cluster.h"
#include "raft/raft.h"
#include "raft/transport_unix.h"
#include "raft/timer_thread.h"
#include <vector>
#include <spdlog/spdlog.h>
#include <thread>
#include <csignal>
namespace raft {
std::atomic<Cluster*> Cluster::global_instance_for_signal_{nullptr};

Cluster::~Cluster() {
    // Destructor ensures nodes are stopped and joined.
    StopAll();
    JoinAll();
    nodes_.clear();
}

void Cluster::CreateNodes(int n) {
    StopAll(); 
    JoinAll();
    nodes_.clear();
    std::vector<type::PeerInfo> peers;
    std::vector<int> peerIds;
    for (int i = 0; i < n; ++i) {
        peers.push_back({i+1, "/tmp/raft-node-" + std::to_string(i+1) + ".sock"});
        peerIds.push_back(i+1);
    }


    for (int i = 0; i < n; ++i) {
        type::PeerInfo self = peers[i];
        std::shared_ptr<IRaftTransport> transport = 
            std::make_shared<RaftTransportUnix>(self, peers);
        std::shared_ptr<ITimerFactory> timerFactory = 
            std::make_shared<ThreadTimerFactory>(); 
        // inject transport into Raft node and create node
        std::shared_ptr<Raft> raftNode = 
            std::make_shared<Raft>(self.id, peerIds,transport,timerFactory);        
        nodes_.push_back(raftNode);
    }
    spdlog::info("Raft cluster with {} nodes initialized.", nodes_.size());
}
void Cluster::StartAll() {
    for (auto &n : nodes_) n->Start();
}

void Cluster::StopAll() {
    for (auto &n : nodes_) n->Stop();
}

void Cluster::JoinAll() {
    for (auto &n : nodes_) n->Join();
}

void Cluster::SignalHandler(int signum) {
    spdlog::info("[Cluster] Caught signal {}.", signum);
    Cluster* inst = global_instance_for_signal_.load();
    if (inst) {
        inst->shutdown_requested_.store(true);
        inst->shutdown_cv_.notify_one();
    }
}

void Cluster::WaitForShutdown() {
    // Register static pointer for signal forwarding.
    global_instance_for_signal_.store(this);

    // Register signal handler (SIGINT for ctrl-c). POSIX signal handling.
    std::signal(SIGINT, Cluster::SignalHandler);
#ifdef SIGTERM
    //SIGTERM:kill
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