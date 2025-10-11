#pragma once
#include "transport.h"
#include "types.h" 
#include <unordered_map>
#include <memory>
#include <vector>


namespace rpc {
    class RpcClient;
    class RpcServer;
    class IMessageCodec;
}

namespace raft {

// Unix-domain-socket-based transport for single-machine multi-process simulation.
class RaftTransportUnix : public IRaftTransport {
public:
    explicit RaftTransportUnix(const raft::type::PeerInfo& self, 
        const std::vector<raft::type::PeerInfo>& peers);
    ~RaftTransportUnix();

    bool RequestVoteRPC(int,const raft::type::RequestVoteArgs&,
    raft::type::RequestVoteArgs&,std::chrono::milliseconds) override;
    bool AppendEntriesRPC(int,const raft::type::AppendEntriesArgs&,
    raft::type::AppendEntriesReply&,std::chrono::milliseconds) override;

    void registerRequestVoteHandler(
        std::function<std::string(const std::string&)> handler) override;
    void registerAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler) override;

private:
    // Information about self and peers
    const raft::type::PeerInfo self_;
    const std::vector<raft::type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<rpc::RpcServer> server_;
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;  // key: peer id


    // RPC handlers
    std::function<std::string(const std::string&)> requestVoteHandler_;
    std::function<std::string(const std::string&)> appendEntriesHandler_;



    std::shared_ptr<rpc::IMessageCodec> codec_;
};

} // namespace raft
