#include "kvstore/kvserver.h"
#include "kvstore/statemachine.h"  
#include "kvstore/codec/kv_codec.h"
// #include "kvstore/types.h"
#include <spdlog/spdlog.h>
#include <optional>
#include <cstdint>  //int32_t

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
    
    rf_->SetApplyCallback(
        [this](raft::type::ApplyMsg& msg){ handleApplyMsg(msg); }
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
bool KVServer::IsReady() const {
    std::lock_guard<std::mutex> lk(mu_);
    return rf_->IsLeader();
}

bool KVServer::isSnapShotEnabled() const{
    std::lock_guard<std::mutex> lk(mu_);
    return isSnapShotEnabledLocked();
}
//---------- Testing utilities ----------
std::shared_ptr<raft::Raft> KVServer::testGetRaftNode() const { return rf_;}
std::shared_ptr<KVStateMachine> KVServer::testGetSM() const{return kvSM_;}
int KVServer::testGetMaxRaftState() const{
    std::lock_guard<std::mutex> lk(mu_);
    return maxRaftState_;
}
int KVServer::testGetLastIncludedIndex() const {
    std::lock_guard<std::mutex> lk(mu_);
    if (rf_) {
        return rf_->testGetLastIncludedIndex();
    }
    return 0;
}
void KVServer::testSetMaxRaftState(int maxRaftState){
    std::lock_guard<std::mutex> lk(mu_);
    maxRaftState_ = maxRaftState;
}
bool KVServer::testMaybeSnapShot(int appliedIndex){
    return maybeTakeSnapshot(appliedIndex);
}
//----------Private RPC handlers----------
void KVServer::PutAppend(
    const type::PutAppendArgs& args,type::PutAppendReply& reply) {
    // spdlog::info("[KVServer] {} PutAppend called: Key={}, Value={}, Op={}", 
        // me_, args.Key, args.Value, args.Op);
    std::lock_guard<std::mutex> lk(mu_);
    //check Idempotency
    auto it = lastAppliedRequestId.find(args.ClientId);
    if(it != lastAppliedRequestId.end() && args.RequestId <= it->second){
        //duplicate checking
        spdlog::info("[KVServer] {} Duplicate PutAppend: ClientId={}, RequestId={}, lastApplied={}", 
            me_, args.ClientId, args.RequestId, it->second);
        reply.err = type::Err::OK;
        return ;
    }
    //first time request
    type::KVCommand command(
        type::KVCommand::String2CommandType(args.Op), 
        args.Key, 
        args.Value,
        args.ClientId,
        args.RequestId
    );
    bool ok = rf_->SubmitCommand(command.ToString());
    if(!ok){reply.err = type::Err::ErrWrongLeader;}
    else{ 
        reply.err = type::Err::OK;
        // do not update idempodency here,only do after state machine applied
        // lastAppliedRequestId[args.ClientId] = args.RequestId;
    }
}
void KVServer::Get(
    const type::GetArgs& args,type::GetReply& reply) {
    // spdlog::info("[KVServer] {} Get called: Key={}", me_, args.Key);
    // type::KVCommand command(
    //     type::KVCommand::CommandType::GET, args.);
    // bool ok = rf_->SubmitCommand(command.ToString());
    // if(!ok){
    //     //for lab3,we simplify get to leader only
    //     reply.err = type::Err::ErrWrongLeader;
    //     return;
    // }
    int32_t currentTerm;
    bool isLeader;
    rf_->GetState(currentTerm,isLeader);
    if(!isLeader){
        //for lab3,we simplify get to leader only
        reply.err = type::Err::ErrWrongLeader;
        return;
    }
    std::lock_guard<std::mutex> lk(mu_);
    std::optional<std::string> value = kvSM_->Get(args.Key);
    if(value){
        spdlog::info("[KVServer] {} Get found [{}] = {}", me_, args.Key, *value);
        reply.Value = *value;
        reply.err = type::Err::OK;
    }else{
        reply.err = type::Err::ErrNoKey;
    }
}
// raft layer is only responsible for copy and broadcast 'command'
// in string bytes,without any ideas about what the hell is 
// KVCommand,Get,PutAppend
void KVServer::handleApplyMsg(raft::type::ApplyMsg& msg){
    if(msg.CommandValid){
        kvSM_->Apply(msg.Command);
        // update after applied to state machine
        type::KVCommand kvCommand = 
            type::KVCommand::FromString(msg.Command);
        if(kvCommand.type != type::KVCommand::CommandType::INVALID){
            std::lock_guard<std::mutex> lk(mu_);
            // spdlog::info("[KVServer] {} lastAppliedRequestId[ClientId={}] = [RequestId={}]", 
            //     me_, kvCommand.ClientId, kvCommand.RequestId);
            lastAppliedRequestId[kvCommand.ClientId] = kvCommand.RequestId;
        }else{ spdlog::warn("[KVServer] {} received invalid command", me_);}
        maybeTakeSnapshot(msg.CommandIndex);
    }else if(msg.SnapshotValid){
        //leader install
        {
            std::lock_guard<std::mutex> lk(mu_);
            kvSM_->ApplySnapShot(msg.Snapshot);
        }
    }else{
        spdlog::info("[KVServer] {} ignored non-command ApplyMsg", me_);
    }
}

bool KVServer::isSnapShotEnabledLocked() const{ return maxRaftState_ != -1;}
bool KVServer::maybeTakeSnapshot(int appliedIndex){
    // spdlog::info("[KVServer] try to acquire snap lock");
    std::lock_guard<std::mutex> lk(mu_);
    // spdlog::info("[KVServer] acquired snap lock");
    //1. snapshot enabled
    if(!isSnapShotEnabledLocked()){
        // spdlog::info("[KVServer] Snapshot not enabled (maxRaftState_={}), skipping snapshot", maxRaftState_);
        return false;
    }
    //2. if Raft persisted state size surpasses threshold
    if(rf_->GetPersistSize() < maxRaftState_){
        spdlog::info("[KVServer] Persist size {} < threshold {}, not snapshotting", 
                      rf_->GetPersistSize(), maxRaftState_);
        return false;
    }

    // 3. Encode the current KV state into a snapshot.
    //    This snapshot should contain all key-value pairs
    //    and any other metadata that is required to restore the state.
    std::string snapshotData = kvSM_->EncodeSnapShot();
    spdlog::info("[KVServer] Snapshot data:\n{}", snapshotData);

    // 4. Instruct Raft to persist the snapshot and compact its log up to 'appliedIndex'.
    //    This tells Raft that log entries ≤ appliedIndex are now safely included in the snapshot
    //    and can be discarded to reduce storage size.
    rf_->SnapShot(appliedIndex, snapshotData);

    // 5. Optionally, log debug info for diagnosis.
    spdlog::info("[KVServer] Took snapshot at index {}, state size = {}, threshold = {}",
                 appliedIndex, rf_->GetPersistSize(), maxRaftState_);
    return true;
}

}// namespace kv