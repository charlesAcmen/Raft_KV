#pragma once
#include "raft/types.h" // for RequestVoteArgs, RequestVoteReply, AppendEntriesArgs, AppendEntriesReply
#include "rpc/types.h"  // for PeerInfo
namespace raft {
// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport{
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
        rpc::type::RPCHandler handler);
    virtual void RegisterAppendEntriesHandler(
        rpc::type::RPCHandler handler); 
protected:
    // RPC handlers
    rpc::type::RPCHandler requestVoteHandler_;
    rpc::type::RPCHandler appendEntriesHandler_;
};


} // namespace raft