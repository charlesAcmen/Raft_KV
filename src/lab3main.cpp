#include "kvstore/kvcluster.h"
#include <spdlog/spdlog.h>
int main(){
    kv::KVCluster cluster(5,0);
    // kv::KVCluster cluster(5,1);
    cluster.StartAll();

    cluster.WaitForServerLeader();
    return 0;
}