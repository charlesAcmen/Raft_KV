#include "kvstore/kvcluster.h"
#include "kvstore/clerk.h"
#include <spdlog/spdlog.h>
int main(){
    // kv::KVCluster cluster(5,0);
    kv::KVCluster cluster(5,1);
    cluster.StartAll();

    cluster.WaitForServerLeader();
    std::shared_ptr<kv::Clerk> clerk = cluster.testGetClerk(0);    
    std::string key = 
    // "Grand Theft Auto V"; 
    "key1";
    std::string value = 
    // "Grand Theft Auto VI";
    "value1";
    clerk->Put(key,value);
    cluster.WaitForShutdown();
    
    return 0;
}