#include "kvstore/kvserver.h"
namespace kv {
KVServer::KVServer(std::shared_ptr<raft::Raft> raft)
    : raft_(raft), kvStateMachine_(std::make_shared<KVStateMachine>()) {
    raft_->SetApplyCallback(
        [this](type::ApplyMsg& msg) {
            kvStateMachine_->Apply(msg.command);
        }
    );
}
void KVServer::Put(const std::string& key, const std::string& value) {
    
}
void KVServer::Append(const std::string& key, const std::string& arg){

}
void KVServer::Get(const std::string& key){

}

}// namespace kv