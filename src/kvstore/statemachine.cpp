#include "kvstore/statemachine.h"
#include "kvstore/types.h"  //KVCommand fromstring tostring
#include <spdlog/spdlog.h>
namespace kv {
KVStateMachine::KVStateMachine(int me):me_(me){}
void KVStateMachine::Apply(const std::string& command) {
    std::lock_guard<std::mutex> lock(mu_);
    spdlog::info("[KVStateMachine] kvserver {} Applied command: {}", me_,command);
    type::KVCommand kvCommand = type::KVCommand::FromString(command);
    switch (kvCommand.type){
    case type::KVCommand::CommandType::GET:
        //usually no-op for state machine
        break;
    case type::KVCommand::CommandType::PUT:
        store_[kvCommand.key] = kvCommand.value;
        spdlog::info("[KVStateMachine] kvserver {} store {} at {}",
            me_,store_[kvCommand.key],kvCommand.key);
        break;
    case type::KVCommand::CommandType::APPEND:{
        std::string before = store_[kvCommand.key];
        store_[kvCommand.key] += kvCommand.value;
        spdlog::info(
            "[KVStateMachine] kvserver {} append before {},after {} at {}",
                me_,before,store_[kvCommand.key],kvCommand.key);
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
        return it->second;
    }
    return std::nullopt;
}
}// namespace kv
