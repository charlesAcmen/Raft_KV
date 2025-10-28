#include "raft/transport.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace raft{
IRaftTransport::IRaftTransport(
    const type::PeerInfo& self,const std::vector<type::PeerInfo>& peers)
    : self_(self), peers_(peers) {}
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