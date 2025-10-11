#pragma once
#include "transport.h"
// #include "rpc/client.h"
// #include "rpc/server.h"
#include "types.h" 
#include <unordered_map>
#include <memory>
#include <vector>


namespace rpc {
    class RpcClient;
    class RpcServer;
}

// namespace raft::type{
//     struct PeerInfo;
//     struct RequestVoteArgs;
//     struct RequestVoteReply;
//     struct AppendEntriesArgs;
//     struct AppendEntriesReply;
// }


namespace raft {

// Unix-domain-socket-based transport for single-machine multi-process simulation.
class RaftTransportUnix : public IRaftTransport {
public:
    RaftTransportUnix(const raft::type::PeerInfo& self, 
        const std::vector<raft::type::PeerInfo>& peers);
    ~RaftTransportUnix();

    bool RequestVoteRPC(int,const raft::type::RequestVoteArgs&,
    raft::type::RequestVoteArgs&,std::chrono::milliseconds) override;

    bool AppendEntriesRPC(int,const raft::type::AppendEntriesArgs&,
    raft::type::AppendEntriesReply&,std::chrono::milliseconds) override;

private:
    const raft::type::PeerInfo self_;
    const std::vector<raft::type::PeerInfo> peers_;

    std::unique_ptr<rpc::RpcServer> server_;
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;  // key: peer id
};

} // namespace raft
