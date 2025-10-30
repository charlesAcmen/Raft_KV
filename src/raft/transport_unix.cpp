#include "raft/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "raft/codec/raft_codec.h"
#include <spdlog/spdlog.h>
namespace rpc{
    class DelimiterCodec;
}
namespace raft{

RaftTransportUnix::RaftTransportUnix(
    const rpc::type::PeerInfo& self, 
    const std::vector<rpc::type::PeerInfo>& peers)
    : TransportBase(self, peers) {}
RaftTransportUnix::~RaftTransportUnix() {
    Stop();
}
void RaftTransportUnix::Start() {
    TransportBase::Start();
}
void RaftTransportUnix::Stop() {
    TransportBase::Stop();
}
bool RaftTransportUnix::RequestVoteRPC(
    int targetId,const type::RequestVoteArgs& args,type::RequestVoteReply& reply) {
    return SendRPC<type::RequestVoteArgs,type::RequestVoteReply>(
        targetId,"Raft.RequestVote",args,reply,
        [](const type::RequestVoteArgs& a) -> std::string{
            return codec::RaftCodec::encode(a);
        },
        codec::RaftCodec::decodeRequestVoteReply
    );
}
bool RaftTransportUnix::AppendEntriesRPC(
    int targetId,const type::AppendEntriesArgs& args,type::AppendEntriesReply& reply) {
    return SendRPC<type::AppendEntriesArgs,type::AppendEntriesReply>(
        targetId,"Raft.AppendEntries",args,reply,
        [](const type::AppendEntriesArgs& a) -> std::string{
            return codec::RaftCodec::encode(a);
        },
        codec::RaftCodec::decodeAppendEntriesReply
    );
}

void RaftTransportUnix::RegisterRequestVoteHandler(
    const rpc::type::RPCHandler& handler) {
    requestVoteHandler_ = std::move(handler);
    TransportBase::RegisterHandler("Raft.RequestVote", handler);
}
void RaftTransportUnix::RegisterAppendEntriesHandler(
    const rpc::type::RPCHandler& handler) {
    appendEntriesHandler_ = std::move(handler);
    TransportBase::RegisterHandler("Raft.AppendEntries", handler);
}
}// namespace raft