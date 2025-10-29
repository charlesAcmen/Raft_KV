#include "raft/transport.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace raft{
IRaftTransport::IRaftTransport(
    const rpc::type::PeerInfo& self,
    const std::vector<rpc::type::PeerInfo>& peers)
    : ITransport(self,peers){}
void IRaftTransport::RegisterRequestVoteHandler(
    std::function<std::string(const std::string&)> handler) {
    requestVoteHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.RequestVote", requestVoteHandler_);
    }
}

void IRaftTransport::RegisterAppendEntriesHandler(
    std::function<std::string(const std::string&)> handler) {
    appendEntriesHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.AppendEntries", appendEntriesHandler_);
    }
}
}// namespace raft