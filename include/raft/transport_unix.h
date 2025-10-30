#pragma once
#include "raft/transport.h" 
#include "rpc/transport.h"
#include "rpc/types.h"
#include <thread>       //for server thread and client thread
namespace raft {
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class RaftTransportUnix 
    : public TransportBase,
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
};

} // namespace raft
