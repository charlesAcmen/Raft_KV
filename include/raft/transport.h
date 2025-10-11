#pragma once

#include <chrono>
#include <functional>
#include <string>

namespace raft {

//forward declarations
namespace type{
    struct RequestVoteArgs;
    struct RequestVoteReply;
    struct AppendEntriesArgs;
    struct AppendEntriesReply;
}

// Transport abstraction used by Raft to send RPCs to peers. 
// Keeping this abstract decouples Raft state-machine logic 
// from the underlying RPC mechanism.
class IRaftTransport {
    public:
    //interface destructor
    virtual ~IRaftTransport() = default;


    // Synchronously call RequestVote on `targetId`
    virtual bool RequestVoteRPC(int targetId,
    const type::RequestVoteArgs& args,
    type::RequestVoteReply& reply,
    std::chrono::milliseconds timeout) = 0;


    // Synchronously call AppendEntries on `targetId`
    virtual bool AppendEntriesRPC(int targetId,
    const type::AppendEntriesArgs& args,
    type::AppendEntriesReply& reply,
    std::chrono::milliseconds timeout) = 0;

    virtual void registerRequestVoteHandler(
        std::function<std::string(const std::string&)> handler) = 0;
    virtual void registerAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler) = 0;   


};


} // namespace raft