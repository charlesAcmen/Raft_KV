#pragma once 
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include "raft/raft.h"
// #include "kvstore/statemachine.h"
namespace kv{
class KVStateMachine; // forward declaration
class KVServer {

public:
    KVServer(int me,std::shared_ptr<raft::Raft> raft,int maxRaftState);

    void StartKVServer();
    void Kill();
    bool Killed() const;


    void PutAppend(const PutAppendArgs& args,PutAppendReply& reply);
    void Get(const GetArgs& args,GetReply& reply);
private:
    mutable std::mutex mu_;
    int me_;
    std::atomic<int32_t> dead_{0};  // set by Kill()    
    std::shared_ptr<raft::Raft> rf_;
    std::shared_ptr<KVStateMachine> kvSM_;
    int maxRaftState_{-1}; //-1 means no snapshotting
};
}// namespace kv
