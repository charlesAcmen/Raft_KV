#include "raft/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include <spdlog/spdlog.h>
namespace raft{
    
raft::RaftTransportUnix::RaftTransportUnix(
    const raft::type::PeerInfo& self, 
    const std::vector<raft::type::PeerInfo>& peers)
    : self_(self), peers_(peers) {
    // Start RPC server
    server_ = std::make_unique<rpc::RpcServer>(self_);
    
    // Create RPC clients for each peer
    for (const auto& peer : peers_) {
        if (peer.id == self_.id) continue; // Skip self
        clients_[peer.id] = std::make_unique<rpc::RpcClient>(self_,peer);
    }
    server_->start();
}
raft::RaftTransportUnix::~RaftTransportUnix() {
    if (server_) {
        server_->stop();
    }
    // peers_.clear();
    clients_.clear();
}
/**
 * @brief Send a RequestVote RPC to a specific peer.
 * 
 * This function only handles the communication logic — 
 * it does NOT implement any voting logic. The Raft layer
 * is responsible for preparing the RequestVoteArgs and
 * handling the returned RequestVoteReply.
 * 
 * @param target_id The peer node ID to which the RPC is sent.
 * @param args      The RequestVoteArgs structure prepared by the Raft instance.
 * @param reply     The RequestVoteReply structure to fill with the peer's response.
 * @return true     If the RPC communication succeeded (not necessarily voted true).
 * @return false    If the RPC failed due to transport errors (e.g. socket closed).
 */
bool raft::RaftTransportUnix::RequestVoteRPC(int targetId,
    const raft::type::RequestVoteArgs& args,
    raft::type::RequestVoteArgs& reply,
    std::chrono::milliseconds timeout) {
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        spdlog::error("[RaftTransportUnix] RequestVoteRPC No RPC client for peer {}", targetId);
        return false;
    }
    rpc::RpcClient* client = it->second.get();//unique_ptr


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
        spdlog::error("[RaftTransportUnix] AppendEntriesRPC No RPC client for peer {}", targetId);
        return false;
    }

    std::string request = "";
    std::string response = it->second->call("AppendEntries", request);
    return true;
}
void raft::RaftTransportUnix::registerRequestVoteHandler(
    std::function<std::string(const std::string&)> handler) {
    server_->register_handler("RequestVote", handler);
}
void raft::RaftTransportUnix::registerAppendEntriesHandler(
    std::function<std::string(const std::string&)> handler) {
    server_->register_handler("AppendEntries", handler);
}

}// namespace raft