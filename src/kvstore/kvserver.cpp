#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
namespace kv {
KVServer::KVServer(int me,std::shared_ptr<raft::Raft> raft,int maxRaftState)
    : rf_(raft), kvSM_(std::make_shared<KVStateMachine>()) {
    rf_->SetApplyCallback(
        [this](raft::type::ApplyMsg& msg) {
            kvSM_->Apply(msg.Command);
        }
    );
    dead_.store(0);
    me_ = me;
    maxRaftState_ = maxRaftState;
}
void KVServer::StartKVServer() {
    rf_->Start();
}


void KVServer::Kill() {
    std::lock_guard<std::mutex> lk(mu_);
    dead_.store(1);
}
bool KVServer::Killed() const {
    std::lock_guard<std::mutex> lk(mu_);
    return dead_.load() != 0;
}

void KVServer::PutAppend(
    const PutAppendArgs& args,PutAppendReply& reply) {
    std::lock_guard<std::mutex> lk(mu_);
}
void KVServer::Get(
    const GetArgs& args,GetReply& reply) {
    std::lock_guard<std::mutex> lk(mu_);
}

}// namespace kv