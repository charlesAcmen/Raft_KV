#include "kvstore/clerk.h"
//type::GetArgs, type::GetReply
//type::PutAppendArgs, type::PutAppendReply
#include "kvstore/types.h"      
#include <spdlog/spdlog.h>
namespace kv {
Clerk::Clerk(int me,const std::vector<int>& peers,
std::shared_ptr<IKVTransport> transport)
    :me_(me),peers_(peers),transport_(transport){
    //do not register any hander for clerk
    //clerk only makes rpc calls
}
Clerk::~Clerk() {transport_->Stop();}
void Clerk::Start(){
    if(started_) return;
    started_ = true;
    transport_->Start();
}
void Clerk::Stop(){
    if(!started_) return;
    started_ = false;
    transport_->Stop();
}
std::string Clerk::Get(const std::string& key) const {
    if(!started_) {
        spdlog::error("[Clerk] {} Get but not started!", me_);
        return "";
    }
    spdlog::info("[Clerk] {} Get key:{}", me_, key);
    
    type::GetArgs args;
    args.Key = key;
    args.ClientId = me_;
    args.RequestId = requestId_++;
    type::GetReply reply;
    while(true){
        if(transport_->GetRPC(me_, args, reply)){
            if(reply.err == type::OK){
                return reply.value;
            }else{
                spdlog::error("[Clerk] {} GetRPC key:{} error:{}", me_, key, reply.err);
                continue;
            }
        }else{
            spdlog::warn("[Clerk] {} GetRPC key:{} error:{}", me_, key, reply.err);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return "";
}
void Clerk::Put(const std::string& key, const std::string& value) {
    PutAppend(key, value, "Put");
}
void Clerk::Append(const std::string& key, const std::string& arg) {
    PutAppend(key, arg, "Append");
}
void Clerk::PutAppend(
    const std::string& key, 
    const std::string& value,
    const std::string op) { 
    if(!started_) {
        spdlog::error("[Clerk] {} PutAppend but not started!", me_);
        return;
    }
    spdlog::info("[Clerk] {} PutAppend key:{} value:{} op:{}", me_, key, value, op);
}
}//namespace kv