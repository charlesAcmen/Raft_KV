#pragma once

#include "raft/types.h"
#include "rpc/types.h"
#include "rpc/transport.h"
#include <functional>   //for rpc handlers
#include <string>
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

    // RPC handlers
    std::function<std::string(const std::string&)> requestVoteHandler_;
    std::function<std::string(const std::string&)> appendEntriesHandler_;
};


} // namespace raft