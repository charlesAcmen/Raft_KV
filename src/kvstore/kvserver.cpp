#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
#include "kvstore/codec/kv_codec.h"
#include <spdlog/spdlog.h>
namespace kv {
KVServer::KVServer(int me,const std::vector<int>& peers,
    std::shared_ptr<IKVTransport> transport,
    std::shared_ptr<raft::Raft> raft,int maxRaftState)
    : me_(me),
    peers_(peers),
    transport_(transport), 
    rf_(raft), 
    kvSM_(std::make_shared<KVStateMachine>()) {
    // Register RPC handlers that accept a serialized payload and return a serialized reply.
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
    maxRaftState_ = maxRaftState;
    spdlog::info("[KVServer] {} created.", me_);
    spdlog::info("[KVServer] {}",maxRaftState_ == -1 ? "No snapshotting." : "Snapshotting enabled.");
}
KVServer::~KVServer() {
    Kill();
    Stop();
    Join();
}
void KVServer::Start() {
    rf_->Start();
    transport_->Start();
}
void KVServer::Stop() {
    transport_->Stop();
    rf_->Shutdown();
}
void KVServer::Kill() {
    if(Killed()) {
        spdlog::warn("[KVServer] {} already killed.", me_);
        return;
    }
    std::lock_guard<std::mutex> lk(mu_);
    dead_.store(1);
}
bool KVServer::Killed() const {
    std::lock_guard<std::mutex> lk(mu_);
    return dead_.load() != 0;
}

void KVServer::PutAppend(
    const type::PutAppendArgs& args,type::PutAppendReply& reply) {
    spdlog::info("[KVServer] {} PutAppend called: Key={}, Value={}, Op={}", me_, args.Key, args.Value, args.Op);

}
void KVServer::Get(
    const type::GetArgs& args,type::GetReply& reply) {
    spdlog::info("[KVServer] {} Get called: Key={}", me_, args.Key);
}

}// namespace kv