#pragma once
#include "raft/types.h" // for RequestVoteArgs, RequestVoteReply, AppendEntriesArgs, AppendEntriesReply
#include "rpc/types.h"  // for handler type
#include "rpc/transport.h" // for rpc::ITransport
namespace raft {
// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport: public rpc::ITransport {
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

    // Synchronously call Installsnapshot on `targetId`
    virtual bool InstallSnapShotRPC(int targetId,
    const type::InstallSnapshotArgs& args,
    type::InstallSnapshotReply& reply) = 0;

    virtual void RegisterRequestVoteHandler(
        const rpc::type::RPCHandler& handler) = 0;
    virtual void RegisterAppendEntriesHandler(
        const rpc::type::RPCHandler& handler) = 0;
    virtual void RegisterInstallSnapShotRPC(
        const rpc::type::RPCHandler& handler) = 0;
protected:
    // RPC handlers
    rpc::type::RPCHandler requestVoteHandler_;
    rpc::type::RPCHandler appendEntriesHandler_;
    rpc::type::RPCHandler installSnapShotHandler_;
};
} // namespace raft