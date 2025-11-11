#pragma once
#include <vector>
#include <memory>               //shared_ptr
#include <condition_variable>
#include <atomic>               //shutdown_requested_ and global_instance_for_signal_
namespace kv{
// forward declaration
class KVServer;
class Clerk;
class KVCluster {
public:
    //number of KV servers and number of clerks
    KVCluster(int,int);
    ~KVCluster();

    void WaitForServerLeader(int maxAttempts = 20); 
    void WaitForShutdown();
    void StartAll();
    void StopAll();
    //------test utilities------
    int testKVServerSize() const;
    int testClerkSize() const;
    std::vector<std::shared_ptr<KVServer>> testGetServers() const;
    std::shared_ptr<KVServer> testGetServer(int index) const;
    std::vector<std::shared_ptr<Clerk>> testGetClerks() const;
    std::shared_ptr<Clerk> testGetClerk(int index) const;
private:
    static constexpr int kSnapshotThreshold = 
        -1;//not enabled
        // 1024 * 1024 * 1;//1MB

    std::vector<std::shared_ptr<KVServer>> kvservers_;
    std::vector<std::shared_ptr<Clerk>> clerks_;

    //wait for shutdown
    std::mutex shutdown_mu_;
    std::condition_variable shutdown_cv_;
    std::atomic<bool> shutdown_requested_{false};

    // static signal handler helper
    static void SignalHandler(int signum);
    // for signal forwarding
    static std::atomic<KVCluster*> global_instance_for_signal_;
};// class KVCluster
}// namespace kv