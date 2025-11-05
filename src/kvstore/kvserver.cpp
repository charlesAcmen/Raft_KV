#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
#include "kvstore/codec/kv_codec.h"
#include <spdlog/spdlog.h>
namespace kv {
KVServer::KVServer(int me,const std::vector<int>& peers,
    std::shared_ptr<IKVTransport> transport,
    std::shared_ptr<raft::Raft> raft,
    int maxRaftState)
    : me_(me),
    peers_(peers),
    transport_(transport), 
    rf_(raft), 
    kvSM_(std::make_shared<KVStateMachine>(me_)){
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
        // raft layer is only responsible for copy and broadcast 'command'
        // in string bytes,without any ideas about what the hell is 
        // KVCommand,Get,PutAppend
        [this](raft::type::ApplyMsg& msg) {
            kvSM_->Apply(msg.Command);
        }
    );
    dead_.store(0);
    maxRaftState_ = maxRaftState;
    // spdlog::info("[KVServer] {} created.", me_);
    // spdlog::info("[KVServer] {}",maxRaftState_ == -1 ? "No snapshotting." : "Snapshotting enabled.");
}
KVServer::~KVServer() { Stop();}
void KVServer::Start() {
    if(Killed()){
        spdlog::warn("[KVServer] {} killed",me_);
        return; //already started
    }
    rf_->Start();
    transport_->Start();
}
void KVServer::Stop() {
    Kill();
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
//---------- Testing utilities ----------
std::shared_ptr<raft::Raft> KVServer::testGetRaftNode() const {
    return rf_;
}
//----------Private RPC handlers----------
void KVServer::PutAppend(
    const type::PutAppendArgs& args,type::PutAppendReply& reply) {
    // spdlog::info("[KVServer] {} PutAppend called: Key={}, Value={}, Op={}", 
        // me_, args.Key, args.Value, args.Op);

    type::KVCommand command(type::KVCommand::String2CommandType(args.Op), args.Key, args.Value);
    bool ok = rf_->SubmitCommand(command.ToString());
    if(!ok){
        reply.err = type::Err::ErrWrongLeader;
        return;
    }
}
void KVServer::Get(
    const type::GetArgs& args,type::GetReply& reply) {
    spdlog::info("[KVServer] {} Get called: Key={}", me_, args.Key);
    type::KVCommand command(
        type::KVCommand::CommandType::GET, args.Key, "");
    bool ok = rf_->SubmitCommand(command.ToString());
    if(!ok){
        reply.err = type::Err::ErrWrongLeader;
    }
    auto value = kvSM_->Get(args.Key);
    if(value){
        spdlog::info("[KVServer] {} Get: found Key={}, Value={}", me_, args.Key, *value);
        reply.Value = *value;
        reply.err = type::Err::OK;
    }else{
        reply.err = type::Err::ErrWrongLeader;
    }
}

}// namespace kv