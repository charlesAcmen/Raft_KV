#include "raft/transport.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace raft{
IRaftTransport::IRaftTransport(
    const rpc::type::PeerInfo& self,
    const std::vector<rpc::type::PeerInfo>& peers)
    : ITransport(self,peers){}
void IRaftTransport::RegisterRequestVoteHandler(
    rpc::type::RPCHandler handler) {
    requestVoteHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.RequestVote", requestVoteHandler_);
    }
}

void IRaftTransport::RegisterAppendEntriesHandler(
    rpc::type::RPCHandler handler) {
    appendEntriesHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("Raft.AppendEntries", appendEntriesHandler_);
    }
}
}// namespace raft