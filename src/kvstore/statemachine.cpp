#include "kvstore/statemachine.h"
#include <spdlog/spdlog.h>
namespace kv {
void KVStateMachine::Apply(const type::KVCommand& command) {
    spdlog::info("Applied command: {}", command.type, command.key, command.value);
    switch(command.type) {
        case type::KVCommand::CommandType::PUT:
            store_[command.key] = command.value;
            break;
        case type::KVCommand::CommandType::APPEND:
            store_[command.key] += command.value;
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
