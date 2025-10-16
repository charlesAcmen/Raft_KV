#pragma once

#include "types.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include <chrono>       //for RPC timeout constexpr
#include <functional>
#include <string>
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
#include <thread>       //for server thread and client thread
namespace raft {
// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport {
public:
    IRaftTransport(const type::PeerInfo& self,
                   const std::vector<type::PeerInfo>& peers)
        : self_(self), peers_(peers) {};
    //interface destructor
    virtual ~IRaftTransport() = default;


    virtual void Start() = 0;
    virtual void Stop() = 0;

    // Synchronously call RequestVote on `targetId`
    virtual bool RequestVoteRPC(int targetId,
    const type::RequestVoteArgs& args,
    type::RequestVoteReply& reply) = 0;


    // Synchronously call AppendEntries on `targetId`
    virtual bool AppendEntriesRPC(int targetId,
    const type::AppendEntriesArgs& args,
    type::AppendEntriesReply& reply) = 0;

    virtual void RegisterRequestVoteHandler(
        std::function<std::string(const std::string&)> handler) = 0;
    virtual void RegisterAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler) = 0;   
protected:
    static constexpr std::chrono::milliseconds RPC_TIMEOUT{500};

    // Information about self and peers
    const type::PeerInfo self_;
    const std::vector<type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<rpc::RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;

    // RPC handlers
    std::function<std::string(const std::string&)> requestVoteHandler_;
    std::function<std::string(const std::string&)> appendEntriesHandler_;
};


} // namespace raft