#include "kvstore/statemachine.h"
namespace kv {
void KVStateMachine::Apply(const std::string& command) {
            
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
