#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
namespace kv {
class KVStateMachine {
public:
    KVStateMachine() = default;
    void Apply(const std::string& command);
    std::optional<std::string> Get(const std::string& key) const;
private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
};//class KVStateMachine
}// namespace kv
