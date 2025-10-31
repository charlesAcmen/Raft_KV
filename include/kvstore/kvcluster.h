#pragma once
#include "kvstore/kvserver.h"
#include "kvstore/clerk.h"
#include <vector>
#include <memory>               //shared_ptr
namespace kv{
class KVCluster {
public:
    KVCluster(int,int);
    ~KVCluster();

    void StartAll();
    void StopAll();
    //------test utilities------
private:
    std::vector<std::shared_ptr<KVServer>> kvservers_;
    std::vector<std::shared_ptr<Clerk>> clerks_;
};// class KVCluster
}// namespace kv