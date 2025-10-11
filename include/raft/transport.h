#pragma once

#include <chrono>

//forward declarations
namespace raft::type{
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
    const raft::type::RequestVoteArgs& args,
    raft::type::RequestVoteArgs& reply,
    std::chrono::milliseconds timeout) = 0;


    // Synchronously call AppendEntries on `targetId`.
    virtual bool AppendEntriesRPC(int targetId,
    const raft::type::AppendEntriesArgs& args,
    raft::type::AppendEntriesReply& reply,
    std::chrono::milliseconds timeout) = 0;

    virtual void registerRequestVoteHandler(
        std::function<std::string(const std::string&)> handler) = 0;
    virtual void registerAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler) = 0;   


};


} // namespace raft