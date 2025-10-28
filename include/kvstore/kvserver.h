#pragma once 
#include <string>
#include <memory>
#include <atomic>
#include "raft/raft.h"
// #include "kvstore/statemachine.h"
namespace kv{
class KVStateMachine; // forward declaration
class KVServer {

public:
    KVServer(int me,std::shared_ptr<raft::Raft> raft,int maxRaftState);

    std::shared_ptr<KVServer> StartKVServer();

    // Called by network RPC from Clerk
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& arg);
    void Get(const std::string& key);
private:
    int me_;
    std::atomic<int32_t> dead_{0};  // set by Kill()    
    std::shared_ptr<raft::Raft> rf_;
    std::shared_ptr<KVStateMachine> kvStateMachine_;
    int maxRaftState_{-1}; //-1 means no snapshotting
};
}// namespace kv
