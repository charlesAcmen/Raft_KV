#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
namespace kv {
class KVStateMachine {
public:
    KVStateMachine(int);
    void Apply(const std::string& command);
    std::optional<std::string> Get(const std::string& key) const;

    // -------------- Lab3 PartB: Snapshot / Compaction ----------------
    std::string EncodeSnapShot() const;
    void ApplySnapShot(const std::string& data);
private:
    mutable std::mutex mu_;
    int me_;    //corresponding kvserver id
    std::unordered_map<std::string, std::string> store_;
};//class KVStateMachine
}// namespace kv
