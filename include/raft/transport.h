#pragma once

#include <chrono>

//forward declarations
namespace rpc{
    struct RequestVoteArgs;
    struct RequestVoteReply;
    struct AppendEntriesArgs;
    struct AppendEntriesReply;
}


namespace raft {


// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport {
    public:
    virtual ~IRaftTransport() = default;


    // Synchronously call RequestVote on `targetId`.
    // Returns true if the RPC reached the remote and `reply` was populated;
    // returns false if the RPC failed due to network (unreliable) conditions.
    // Implementations must respect the `timeout` (best-effort).
    virtual bool RequestVoteRPC(int targetId,
    const rpc::RequestVoteArgs& args,
    rpc::RequestVoteArgs& reply,
    std::chrono::milliseconds timeout) = 0;


    // Synchronously call AppendEntries on `targetId`.
    virtual bool AppendEntriesRPC(int targetId,
    const rpc::AppendEntriesArgs& args,
    rpc::AppendEntriesReply& reply,
    std::chrono::milliseconds timeout) = 0;
};


} // namespace raft