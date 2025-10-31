#include "kvstore/kvcluster.h"
#include <spdlog/spdlog.h>
int main(){
    kv::KVCluster cluster(5,1);
    cluster.StartAll();
    return 0;
}