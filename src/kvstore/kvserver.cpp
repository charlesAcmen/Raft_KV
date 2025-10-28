#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
namespace kv {
KVServer::KVServer(int me,std::shared_ptr<raft::Raft> raft,int maxRaftState)
    : rf_(raft), kvStateMachine_(std::make_shared<KVStateMachine>()) {
    rf_->SetApplyCallback(
        [this](raft::type::ApplyMsg& msg) {
            kvStateMachine_->Apply(msg.Command);
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