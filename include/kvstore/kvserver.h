#pragma once 
#include <string>
#include <memory>
#include "raft/raft.h"
#include "kvstore/statemachine.h"
namespace kv{
class KVServer {
public:
    KVServer(std::shared_ptr<raft::Raft> raft);
    // Public API - exported as RPC handlers
    // Called by network RPC from Clerk
    void PUT(const std::string& key, const std::string& value);
    void APPEND(const std::string& key, const std::string& arg);
    void GET(const std::string& key);
private:
    std::shared_ptr<raft::Raft> raft_;
    std::shared_ptr<KVStateMachine> kvStateMachine_;
};
}// namespace kv
