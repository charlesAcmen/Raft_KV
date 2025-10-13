#include "raft/cluster.h"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
int main(){
    spdlog::info("Starting cluster example");
    raft::Cluster cluster;
    cluster.CreateNodes(3);

    // Start all nodes. They will run in background threads.
    cluster.StartAll();

    // (optional) run some short simulation then continue to wait for Ctrl+C
    std::this_thread::sleep_for(std::chrono::seconds(3));
    spdlog::info("3 seconds passed, cluster still running. Press Ctrl+C to stop.");

    // This will block until user presses Ctrl+C (SIGINT),
    // then Cluster will StopAll() + JoinAll() before returning.
    cluster.WaitForShutdown();

    spdlog::info("Cluster stopped, main returns.");
    return 0;




    // 1. peers info
    // std::vector<raft::type::PeerInfo> peers = {
    //     {1, "/tmp/raft-node-1.sock"},
    //     {2, "/tmp/raft-node-2.sock"},
    //     {3, "/tmp/raft-node-3.sock"}
    // };
    // std::vector<int> peerIds;
    // for (const auto& p : peers) {
    //     peerIds.push_back(p.id);
    // }
    // // 2. raft nodes
    // std::vector<std::shared_ptr<raft::Raft>> nodes;

    // for (const auto& self : peers) {
    //     // transport for every node
    //     std::shared_ptr<raft::IRaftTransport> transport = std::make_shared<raft::RaftTransportUnix>(self, peers);
    //     std::shared_ptr<raft::ITimerFactory> timerFactory = std::make_shared<raft::ThreadTimerFactory>(); 
    //     // inject transport into Raft node and create node
    //     std::shared_ptr<raft::Raft> raftNode = std::make_shared<raft::Raft>(self.id, peerIds,transport,timerFactory);
    //     nodes.push_back(raftNode);
    // }
    // spdlog::info("Raft cluster with {} nodes initialized.", nodes.size());
    // //3. start all nodes
    // for(auto& node : nodes){
    //     node->Start();
    // }
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    // return 0;
}
