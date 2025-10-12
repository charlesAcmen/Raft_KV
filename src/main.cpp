#include <spdlog/spdlog.h>
#include <iostream>
#include "raft/raft.h"
#include "raft/transport_unix.h"
int main() {
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
    auto applyCallback = [](const raft::type::LogEntry& entry) {
    std::cout << "Applied log: index=" << entry.index 
              << ", term=" << entry.term 
              << ", command=" << entry.command << std::endl;
    };

    for (const auto& self : peers) {
        // transport for every node
        std::shared_ptr<raft::IRaftTransport> transport = std::make_shared<raft::RaftTransportUnix>(self, peers);

        // inject transport into Raft node and create node
        auto raftNode = std::make_shared<raft::Raft>(self.id, peerIds,transport, applyCallback);
        nodes.push_back(raftNode);
    }

    // 3. start all nodes
    // for (auto& node : nodes) {
    //     node->start();
    // }
    return 0;
}
