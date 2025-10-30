#pragma once
#include "raft/transport.h" 
#include "rpc/transport.h"
#include "rpc/types.h"
#include <thread>       //for server thread and client thread
namespace raft {
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class RaftTransportUnix 
    : public rpc::TransportBase,
      public IRaftTransport {
public:
    explicit RaftTransportUnix(
        const rpc::type::PeerInfo&,
        const std::vector<rpc::type::PeerInfo>&);
    ~RaftTransportUnix() override;

    bool RequestVoteRPC(
        int,const type::RequestVoteArgs&,type::RequestVoteReply&) override;
    bool AppendEntriesRPC(
        int,const type::AppendEntriesArgs&,type::AppendEntriesReply&) override;
    
    virtual void RegisterRequestVoteHandler(
        rpc::type::RPCHandler handler);
    virtual void RegisterAppendEntriesHandler(
        rpc::type::RPCHandler handler);  
private:
    // RPC handlers
    rpc::type::RPCHandler requestVoteHandler_;
    rpc::type::RPCHandler appendEntriesHandler_;
};

} // namespace raft
