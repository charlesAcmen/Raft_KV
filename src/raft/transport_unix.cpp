#include "raft/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "raft/codec/raft_codec.h"
#include <spdlog/spdlog.h>
namespace rpc{
    class DelimiterCodec;
}
namespace raft{

RaftTransportUnix::RaftTransportUnix(
    const rpc::type::PeerInfo& self, 
    const std::vector<rpc::type::PeerInfo>& peers)
    : TransportBase(self, peers) {}
RaftTransportUnix::~RaftTransportUnix() {
    Stop();
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
bool RaftTransportUnix::RequestVoteRPC(
    int targetId,const type::RequestVoteArgs& args,type::RequestVoteReply& reply) {
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        spdlog::error("[RaftTransportUnix] RequestVoteRPC No RPC client for peer {}", targetId);
        return false;
    }
    rpc::RpcClient* client = it->second.get();//unique_ptr

    //convert args to string as request
    std::string request = codec::RaftCodec::encode(args);
    std::string response = client->Call("Raft.RequestVote", request);
    reply = codec::RaftCodec::decodeRequestVoteReply(response);
    return true;
}
/**
 * @brief Send an AppendEntries RPC to a specific peer.
 * 
 * This function only performs network-level transmission 
 * using the underlying RPC client abstraction. It does not 
 * contain any Raft consensus or log replication logic.
 * 
 * @param target_id The peer node ID to which the RPC is sent.
 * @param args      The AppendEntriesArgs structure prepared by the Raft instance.
 * @param reply     The AppendEntriesReply structure to fill with the peer's response.
 * @return true     If the RPC communication succeeded.
 * @return false    If the RPC transmission failed.
 */
bool RaftTransportUnix::AppendEntriesRPC(
    int targetId,const type::AppendEntriesArgs& args,type::AppendEntriesReply& reply) {
    // spdlog::info("[RaftTransportUnix] {} Sending AppendEntriesRPC to peer {}", self_.id, targetId);
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        spdlog::error("[RaftTransportUnix] AppendEntriesRPC No RPC client for peer {}", targetId);
        return false;
    }
    rpc::RpcClient* client = it->second.get();//unique_ptr

    std::string request = codec::RaftCodec::encode(args);
    std::string response = client->Call("Raft.AppendEntries", request);
    reply = codec::RaftCodec::decodeAppendEntriesReply(response);
    return true;
}


void RaftTransportUnix::RegisterRequestVoteHandler(
    rpc::type::RPCHandler handler) {
    requestVoteHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.RequestVote", requestVoteHandler_);
    }
}

void RaftTransportUnix::RegisterAppendEntriesHandler(
    rpc::type::RPCHandler handler) {
    appendEntriesHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.AppendEntries", appendEntriesHandler_);
    }
}
}// namespace raft