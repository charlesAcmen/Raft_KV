#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
namespace kv {
KVServer::KVServer(int me,
    std::shared_ptr<IKVTransport> transport,
    std::shared_ptr<raft::Raft> raft,
    int maxRaftState)
    : transport_(transport), rf_(raft), kvSM_(std::make_shared<KVStateMachine>()) {
    
    // -----------------------
    // Register RPC handlers
    // -----------------------
    // register RPC handlers that accept a serialized payload and return a serialized reply.
    // The lambdas: decode -> call local handler function -> encode reply.
    transport_->RegisterGetHandler(
        [this](const std::string& payload) -> std::string {
            try {
                type::GetArgs args = codec::KVCodec::decodeGetArgs(payload);
                type::GetReply reply;
                this->Get(args, reply);
                return codec::KVCodec::encode(reply);
            } catch (const std::exception& e) {
                spdlog::error("[KVServer] {} Get handler exception: {}", this->me_, e.what());
                return std::string();
            }
        }
    );
    transport_->RegisterPutAppendHandler(
        [this](const std::string& payload) -> std::string {
            try {
                type::PutAppendArgs args = codec::KVCodec::decodePutAppendArgs(payload);
                type::PutAppendReply reply;
                this->PutAppend(args, reply);
                return codec::KVCodec::encode(reply);
            } catch (const std::exception& e) {
                spdlog::error("[KVServer] {} PutAppend handler exception: {}", this->me_, e.what());
                return std::string();
            }
        }
    );
    
    rf_->SetApplyCallback(
        [this](raft::type::ApplyMsg& msg) {
            kvSM_->Apply(msg.Command);
        }
    );
    dead_.store(0);
    me_ = me;
    maxRaftState_ = maxRaftState;
}
KVServer::~KVServer() {
    Kill();
    rf_->Stop();
    rf_->Join();
    transport_->Stop();
}
void KVServer::StartKVServer() {
    rf_->Start();
    transport_->Start();
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
    const type::PutAppendArgs& args,type::PutAppendReply& reply) {
    std::lock_guard<std::mutex> lk(mu_);

}
void KVServer::Get(
    const type::GetArgs& args,type::GetReply& reply) {
    std::lock_guard<std::mutex> lk(mu_);
}

}// namespace kv