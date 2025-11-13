#include "kvstore/statemachine.h"
#include "kvstore/types.h"  //KVCommand fromstring tostring
#include <spdlog/spdlog.h>
#include <sstream>
namespace kv {
KVStateMachine::KVStateMachine(int me):me_(me){}
void KVStateMachine::Apply(const std::string& command) {
    std::lock_guard<std::mutex> lock(mu_);
    // spdlog::info("[KVStateMachine] {} Applied command: {}", me_,command);
    type::KVCommand kvCommand = type::KVCommand::FromString(command);
    switch (kvCommand.type){
        case type::KVCommand::CommandType::GET:
            //usually no-op for state machine
            break;
        case type::KVCommand::CommandType::PUT:
            store_[kvCommand.key] = kvCommand.value;
            spdlog::info("[KVStateMachine] {} [{}] = {}",
                me_,kvCommand.key,store_[kvCommand.key]);
            break;
        case type::KVCommand::CommandType::APPEND:{
            // std::string before = store_[kvCommand.key];
            store_[kvCommand.key] += kvCommand.value;
            // spdlog::info("[KVStateMachine] kvserver {} append before {},after {} at {}",
            //         me_,before,store_[kvCommand.key],kvCommand.key);
            spdlog::info("[KVStateMachine] {} [{}] = {}",
                me_,kvCommand.key,store_[kvCommand.key]);
            break;
        }
        default:
            break;
    }
}
std::optional<std::string> KVStateMachine::Get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        // spdlog::info("[KVStateMachine] kvserver {} Get [{}] = {}", 
        //     me_, key, it->second);
        return it->second;
    }
    // spdlog::info("[KVStateMachine] kvserver {} Get key {} not found", 
    //     me_, key);
    return std::nullopt;
}
std::string KVStateMachine::EncodeSnapShot() const {
    std::stringstream ss;
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& [k, v] : store_) {
        ss << k << '\n' << v << '\n';
    }
    return ss.str();
}
void KVStateMachine::ApplySnapShot(const std::string& data){
    std::lock_guard<std::mutex> lock(mu_);

    //clear old state
    store_.clear();

    //install kv pairs from serialized data
    std::stringstream ss(data);
    std::string key,value;
    while(true){
        if(!std::getline(ss,key,'\n')) break;
        if(!std::getline(ss,value,'\n')) break;
        store_[key] = value;
    }
    spdlog::info("[KVStateMachine] snapshot installed, restored {} keys", 
        store_.size());
}


//---------- Testing utilities ----------
void KVStateMachine::testApply(const std::string& command){
    Apply(command);
}
}// namespace kv
