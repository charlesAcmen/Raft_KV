#pragma once
#include <vector>
#include <memory>               //shared_ptr
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
    void StartAll();
    void StopAll();
    //------test utilities------
private:
    std::vector<std::shared_ptr<KVServer>> kvservers_;
    std::vector<std::shared_ptr<Clerk>> clerks_;
};// class KVCluster
}// namespace kv