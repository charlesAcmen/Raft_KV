#pragma once 
#include "raft/raft.h"          // for raft::Raft
#include "kvstore/types.h"      // for type::PutAppendArgs, type::GetArgs, etc
#include "kvstore/transport.h"  // for IKVTransport
#include <string>
#include <memory>       //shared_ptr
#include <atomic>       //dead_
#include <mutex>
namespace kv{
class KVStateMachine; // forward declaration
class KVServer {
public:
    KVServer(int,const std::vector<int>&,
        std::shared_ptr<IKVTransport>,
        std::shared_ptr<raft::Raft>,
        int = -1);
    ~KVServer();
    void Start();
    void Stop();
    void Kill();
    bool Killed() const;
    bool isSnapShotEnabled() const;

    //---------- Testing utilities ----------
    std::shared_ptr<raft::Raft> testGetRaftNode() const;
private:
    void PutAppend(const type::PutAppendArgs& args,type::PutAppendReply& reply);
    void Get(const type::GetArgs& args,type::GetReply& reply);

    mutable std::mutex mu_;
    const int me_;                      // this peer's id (index into peers_)
    const std::vector<int> peers_;      // peer ids (including me_)
    std::atomic<int32_t> dead_{1};  // set by Kill()，0：not killed
    
    //key:clerk id,value:last applied requestId
    std::unordered_map<int, int> lastAppliedRequestId;  
    
    // used to receive rpcs from Clerk and handle them
    std::shared_ptr<IKVTransport> transport_; 
    std::shared_ptr<raft::Raft> rf_;
    std::shared_ptr<KVStateMachine> kvSM_;

    // -------------- Lab3 PartB: Snapshot / Compaction ----------------
    int maxRaftState_{-1}; // Threshold for log size to trigger snapshot, -1 = disabled

    bool isSnapShotEnabledLocked() const;
    void maybeTakeSnapshot(int appliedIndex);
};
}// namespace kv
