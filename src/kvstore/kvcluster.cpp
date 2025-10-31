#include "kvstore/kvcluster.h"
namespace kv{
KVCluster::KVCluster(int numServers, int numClerks) {

}
KVCluster::~KVCluster() {
    StopAll();
    JoinAll();
}
void KVCluster::StartAll() {
    for (auto &svr : kvservers_) svr->Start();
    for (auto &ck : clerks_) ck->Start();
}
void KVCluster::StopAll() {
    for (auto &svr : kvservers_) svr->Stop();
    for (auto &ck : clerks_) ck->Stop();
}
}// namespace kv