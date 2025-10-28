#include "kvstore/kvserver.h"
namespace kv {
KVServer::KVServer(std::shared_ptr<raft::Raft> raft)
    : raft_(raft), kvStateMachine_(std::make_shared<KVStateMachine>()) {
    raft_->SetApplyCallback(
        [this](type::ApplyMsg& msg) {
            if (msg.commandValid) {
                kvStateMachine_->Apply(msg.command);
            }
        }
    );
}
void KVServer::PUT(const std::string& key, const std::string& value) {
    // Implementation of PUT operation
}
void KVServer::APPEND(const std::string& key, const std::string& arg){

}
void KVServer::GET(const std::string& key){

}

}// namespace kv