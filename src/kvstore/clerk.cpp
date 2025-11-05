#include "kvstore/clerk.h"
//type::GetArgs, type::GetReply
//type::PutAppendArgs, type::PutAppendReply
#include "kvstore/types.h"      
#include <spdlog/spdlog.h>
namespace kv {
Clerk::Clerk(int me,const std::vector<int>& peers,
std::shared_ptr<IKVTransport> transport)
    :clerkId_(me),peers_(peers),transport_(transport){
    //do not register any hander for clerk
    //clerk only makes rpc calls
}
Clerk::~Clerk() {transport_->Stop();}
void Clerk::Start(){
    std::lock_guard<std::mutex> lg(mu_);
    if(started_) return;
    started_ = true;
    transport_->Start();
}
void Clerk::Stop(){
    std::lock_guard<std::mutex> lg(mu_);
    if(!started_) return;
    started_ = false;
    transport_->Stop();
}
std::string Clerk::Get(const std::string& key){
    std::lock_guard<std::mutex> lg(mu_);
    if(!started_) {
        spdlog::error("[Clerk] {} Get but not started!", clerkId_);
        return "";
    }
    // spdlog::info("[Clerk] {} Get key:{}", clerkId_, key);
    
    int startServerIdx = lastKnownLeaderIdx_;
    int tried = 0;
    while(true){
        int serverIdx = (startServerIdx + tried++) % peers_.size();
        int serverId = peers_[serverIdx];
        type::GetArgs args{key, clerkId_, nextRequestId_++};
        type::GetReply reply;
        if(transport_->GetRPC(serverId, args, reply)){
            if(reply.err == type::Err::OK){
                lastKnownLeaderIdx_ = serverIdx;
                spdlog::info("[Clerk] {} GetRPC succeeded after {} attempts. Leader server: {}, key: {}", 
                    clerkId_, tried, serverId, key);
                return reply.Value;
            }else if(reply.err == type::Err::ErrWrongLeader){
                // spdlog::info("[Clerk] {} GetRPC key:{} error:{}", 
                // clerkId_, key,type::ErrToString(reply.err));
                continue;
            }
        }else{
            spdlog::warn("[Clerk] {} GetRPC key:{} transport failure, server id:{}", clerkId_, key, serverId);
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return "";
}
void Clerk::Put(const std::string& key, const std::string& value) { PutAppend(key, value, "Put");}
void Clerk::Append(const std::string& key, const std::string& arg) { PutAppend(key, arg, "Append");}
void Clerk::PutAppend(
    const std::string& key, 
    const std::string& value,
    const std::string op) { 
    std::lock_guard<std::mutex> lg(mu_);
    if(!started_) {
        spdlog::error("[Clerk] {} PutAppend but not started!", clerkId_);
        return;
    }
    // spdlog::info("[Clerk] {} PutAppend key:{} value:{} op:{}", clerkId_, key, value, op);
    
    int startServerIdx = lastKnownLeaderIdx_;
    int tried = 0;
    while(true){
        int serverIdx = (startServerIdx + tried++) % peers_.size();
        int serverId = peers_[serverIdx];
        type::PutAppendArgs args{key, value, op, clerkId_, nextRequestId_++};
        type::PutAppendReply reply;
        if(transport_->PutAppendRPC(serverId, args, reply)){
            if(reply.err == type::Err::OK){
                lastKnownLeaderIdx_ = serverIdx;
                spdlog::info("[Clerk] {} PutAppendRPC succeeded after {} attempts. Leader server: {},{},{},{}", 
                    clerkId_, tried, serverId, key, value, op);
                return;
            }else if(reply.err == type::Err::ErrWrongLeader){
                // spdlog::info("[Clerk] {} PutAppendRPC key:{} error:{}", clerkId_, key,type::ErrToString(reply.err));
                continue;
            }
        }else{
            spdlog::warn("[Clerk] {} PutAppendRPC key:{} transport failure, server id:{}", clerkId_, key, serverId);
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
}//namespace kv