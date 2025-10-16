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
    const type::PeerInfo& self, 
    const std::vector<type::PeerInfo>& peers)
    : IRaftTransport(self, peers) {
    // Start RPC server
    server_ = std::make_unique<rpc::RpcServer>(self_);
    
    // Create RPC clients for each peer
    for (const auto& peer : peers_) {
        if (peer.id == self_.id) continue; // Skip self
        clients_[peer.id] = std::make_unique<rpc::RpcClient>(self_,peer);
    }
    // do not start the server here, because the handlers are not registered yet
    // and node will be blocked when starting the server
    // server_->start();
}
RaftTransportUnix::~RaftTransportUnix() {
    Stop();
}

// Start the transport (start RPC server and prepare clients)
void RaftTransportUnix::Start() {
    // Start the server in background
    serverThread_ = std::thread([this]() { server_->Start(); });

    clientThread_ = std::thread([this]() {
        while(true){
            bool allConnected = true;
            for (auto& [id, client] : clients_) {
                if(client->Connect()){
                    // spdlog::info("[RaftTransportUnix] {} Connected to peer {}", this->self_.id, id);
                }else{
                    allConnected = false;
                }
            }
            if(allConnected) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        spdlog::info("[RaftTransportUnix] {}:All RPC clients connected to peers",self_.id);
    });
}

// Shutdown transport gracefully
void RaftTransportUnix::Stop() {
    if (server_) server_->Stop();
    for (auto& [id, client] : clients_) {
        client->Close();
    }
    if (serverThread_.joinable()) serverThread_.join();
    if (clientThread_.joinable()) clientThread_.join();
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
    // spdlog::info("[RaftTransportUnix] {} Sending RequestVoteRPC to peer {}", self_.id, targetId);
    auto it = clients_.find(targetId);
    if (it == clients_.end()) {
        spdlog::error("[RaftTransportUnix] RequestVoteRPC No RPC client for peer {}", targetId);
        return false;
    }
    rpc::RpcClient* client = it->second.get();//unique_ptr

    //convert args to string as request
    std::string request = codec::RaftCodec::encode(args);
    std::string response = client->Call("RequestVote", request);
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
    std::string response = client->Call("AppendEntries", request);
    reply = codec::RaftCodec::decodeAppendEntriesReply(response);
    return true;
}
}// namespace raft