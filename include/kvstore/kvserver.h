#pragma once 
#include <string>
#include <memory>
#include <atomic>
// #include "raft/raft.h"
// #include "kvstore/statemachine.h"
namespace kv{
class KVStateMachine; // forward declaration
class raft::Raft; // forward declaration
class KVServer {

public:
    KVServer(int me,std::shared_ptr<raft::Raft> raft,int maxRaftState);
    // Public API - exported as RPC handlers
    // Called by network RPC from Clerk
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& arg);
    void Get(const std::string& key);
private:
    int me_;
    std::atomic<bool> dead_{false};
    std::shared_ptr<raft::Raft> rf;
    std::shared_ptr<KVStateMachine> kvStateMachine_;
    int maxRaftState_{-1}; //-1 means no snapshotting
};
}// namespace kv
