#pragma once

#include "raft/types.h"
#include "rpc/types.h"
#include "rpc/transport.h"
#include <chrono>       //for RPC timeout constexpr
#include <functional>   //for rpc handlers
#include <string>
#include <vector>       //for list of peers
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
namespace rpc {
    class RpcClient;
    class RpcServer;
}

namespace raft {
// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport : public rpc::ITransport {
public:
    virtual ~IRaftTransport() = default;

    // Synchronously call RequestVote on `targetId`
    virtual bool RequestVoteRPC(int targetId,
    const type::RequestVoteArgs& args,
    type::RequestVoteReply& reply) = 0;

    // Synchronously call AppendEntries on `targetId`
    virtual bool AppendEntriesRPC(int targetId,
    const type::AppendEntriesArgs& args,
    type::AppendEntriesReply& reply) = 0;

    virtual void RegisterRequestVoteHandler(
        std::function<std::string(const std::string&)> handler);
    virtual void RegisterAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler);   
protected:
    IRaftTransport(const rpc::type::PeerInfo&,const std::vector<rpc::type::PeerInfo>&);
    static constexpr std::chrono::milliseconds RPC_TIMEOUT{500};

    // Information about self and peers
    const rpc::type::PeerInfo self_;
    const std::vector<rpc::type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<rpc::RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;

    // RPC handlers
    std::function<std::string(const std::string&)> requestVoteHandler_;
    std::function<std::string(const std::string&)> appendEntriesHandler_;
};


} // namespace raft