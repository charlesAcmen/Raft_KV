#include <spdlog/spdlog.h>
#include <iostream>
#include "raft/raft.h"
#include "raft/transport_unix.h"
#include  "raft/timer_thread.h"
#include  <thread>

int main(){
    // 1. peers info
    std::vector<raft::type::PeerInfo> peers = {
        {1, "/tmp/raft-node-1.sock"},
        {2, "/tmp/raft-node-2.sock"},
        {3, "/tmp/raft-node-3.sock"}
    };
    std::vector<int> peerIds;
    for (const auto& p : peers) {
        peerIds.push_back(p.id);
    }


    // 2. raft nodes
    std::vector<std::shared_ptr<raft::Raft>> nodes;

    for (const auto& self : peers) {
        // transport for every node
        std::shared_ptr<raft::IRaftTransport> transport = std::make_shared<raft::RaftTransportUnix>(self, peers);
        std::shared_ptr<raft::ITimerFactory> timerFactory = std::make_shared<raft::ThreadTimerFactory>(); 
        // inject transport into Raft node and create node
        std::shared_ptr<raft::Raft> raftNode = std::make_shared<raft::Raft>(self.id, peerIds,transport,timerFactory);
        nodes.push_back(raftNode);
    }
    spdlog::info("Raft cluster with {} nodes initialized.", nodes.size());
    
    //3. start all nodes
    for(auto& node : nodes){
        node->Start();
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 0;
}
