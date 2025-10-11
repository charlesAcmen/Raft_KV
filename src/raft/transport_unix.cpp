#include "raft/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
raft::RaftTransportUnix::RaftTransportUnix(
    const raft::type::PeerInfo& self, 
    const std::vector<raft::type::PeerInfo>& peers)
    : self_(self), peers_(peers) {
    // Start RPC server
    server_ = std::make_unique<rpc::RpcServer>(self_.sockPath);
    // server_->register_handler("RequestVote", 
    //     [this](const raft::type::RequestVoteArgs& args, raft::type::RequestVoteArgs& reply) {
    //         // This is a placeholder implementation. Actual logic should be implemented in Raft class.
    //         reply.term = args.term;  // Echo back the term for testing
    //         reply.voteGranted = true; // Always grant vote for testing
    //         return 0; // success
    //     });
    // server_->register_handler("AppendEntries", 
    //     [this](const raft::type::AppendEntriesArgs& args, raft::type::AppendEntriesReply& reply) {
    //         // This is a placeholder implementation. Actual logic should be implemented in Raft class.
    //         reply.term = args.term;  // Echo back the term for testing
    //         reply.success = true;    // Always succeed for testing
    //         return 0; // success
    //     });
    // server_->start();

    // Create RPC clients for each peer
    for (const auto& peer : peers_) {
        if (peer.id == self_.id) continue; // Skip self
        clients_[peer.id] = std::make_unique<rpc::RpcClient>(peer.sockPath);
    }
}
raft::RaftTransportUnix::~RaftTransportUnix() {
    if (server_) {
        server_->stop();
    }
    // peers_.clear();
    clients_.clear();
}

bool raft::RaftTransportUnix::RequestVoteRPC(int targetId,
    const raft::type::RequestVoteArgs& args,
    raft::type::RequestVoteArgs& reply,
    std::chrono::milliseconds timeout) {
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        return false;
    }
    
    std::string request = "";
    std::string response = it->second->call("RequestVote", request);
    return true;
}

bool raft::RaftTransportUnix::AppendEntriesRPC(int targetId,
    const raft::type::AppendEntriesArgs& args,
    raft::type::AppendEntriesReply& reply,
    std::chrono::milliseconds timeout) {
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        return false;
    }

    std::string request = "";
    std::string response = it->second->call("AppendEntries", request);
    return true;
}