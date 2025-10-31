#include "kvstore/kvcluster.h"
namespace kv{
KVCluster::KVCluster(int numServers, int numClerks) {

}
KVCluster::~KVCluster() {
    StopAll();
    JoinAll();
    kvservers_.clear();
    clerks_.clear();
}
void KVCluster::StartAll() {
    for (auto &svr : kvservers_) svr->StartKVServer();
    for (auto &ck : clerks_) ck->Start();
}
void KVCluster::StopAll() {
    for (auto &svr : kvservers_) svr->Kill();
    for (auto &ck : clerks_) ck->Stop();
}
void KVCluster::JoinAll() {
    for (auto &ck : clerks_) ck->Stop();
}
}// namespace kv