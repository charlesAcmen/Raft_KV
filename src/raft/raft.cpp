#include "raft/raft.h"
namespace raft{
Raft::Raft(int me,
    const std::vector<raft::type::PeerInfo>& peers,
    std::shared_ptr<IRaftTransport> transport,
    std::function<void(const type::LogEntry&)> applyCallback,
    RaftConfig config,
    std::shared_ptr<ITimerFactory> timerFactory,
    std::shared_ptr<IPersister> persister)
    :me_(me), peers_(peers), transport_(transport),
    applyCallback_(applyCallback), config_(config),
    timerFactory_(timerFactory), persister_(persister) {
    transport_->registerRequestVoteHandler(
        [this](const std::string& req) {
            type::RequestVoteArgs args;
            args.ParseFromString(req);
            type::RequestVoteReply reply;
            this->HandleRequestVote(args, reply);
            std::string resp;
            reply.SerializeToString(&resp);
            return resp;
        });
    transport_->registerAppendEntriesHandler(
        [this](const std::string& req) {
            type::AppendEntriesArgs args;
            args.ParseFromString(req);
            type::AppendEntriesReply reply;
            this->HandleAppendEntries(args, reply);
            std::string resp;
            reply.SerializeToString(&resp);
            return resp;
        });
}
}