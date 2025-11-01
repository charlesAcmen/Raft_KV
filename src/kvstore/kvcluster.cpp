#include "kvstore/kvcluster.h"
#include "kvstore/kvserver.h"       //complete definition of KVServer
#include "kvstore/clerk.h"          //complete definition of Clerk
#include "rpc/types.h"              //rpc::type::PeerInfo
#include "kvstore/transport_unix.h"      //IKVTransport and KVTransportUnix
#include "raft/cluster.h"        //raft::Cluster static CreateRaftNodes
namespace kv{
KVCluster::KVCluster(int numServers, int numClerks) {
    //preparing peer info
    std::vector<rpc::type::PeerInfo> peers;
    std::vector<int> peerIds;
    //first number of clerks, then number of servers
    int numNodes = numServers+numClerks;
    for (int i = 0; i < numNodes; ++i) {
        peers.push_back({i+1, "/tmp/kvnode-" + std::to_string(i+1) + ".sock"});
        peerIds.push_back(i+1);
    }
    for (int i = 0; i < numClerks; ++i) {
        rpc::type::PeerInfo self = peers[i];
        std::shared_ptr<IKVTransport> transport = 
            std::make_shared<KVTransportUnix>(self, peers);
        // inject transport into Clerk and create clerk
        std::shared_ptr<Clerk> clerk = 
            std::make_shared<Clerk>(self.id, peerIds, transport);        
        clerks_.push_back(clerk);
    }
    std::vector<std::shared_ptr<raft::Raft>> raftNodes = raft::Cluster::CreateRaftNodes(numServers);
    for (int i = numClerks; i < numNodes; ++i) {
        rpc::type::PeerInfo self = peers[i];
        std::shared_ptr<IKVTransport> transport = 
        // KVServer does not send RPCs
        // so no clients needed for each kvserver
            std::make_shared<KVTransportUnix>(self, std::vector<rpc::type::PeerInfo>{});
        // inject transport into KV server and create server
        std::shared_ptr<KVServer> kvserver = 
            std::make_shared<KVServer>(self.id, peerIds, transport, raftNodes[i-numClerks], -1);        
        kvservers_.push_back(kvserver);
    }
    spdlog::info("KVCluster with {} servers and {} clerks initialized.", kvservers_.size(), clerks_.size());
}
KVCluster::~KVCluster() {
    StopAll();
}
void KVCluster::WaitForServerLeader(int maxAttempts) {
    for (int i = 0; i < maxAttempts; ++i) {
        for (const auto& svr : kvservers_) {
            int32_t term;
            bool isLeader;
            svr->testGetRaftNode()->GetState(term, isLeader);
            if (isLeader) {
                spdlog::info("[KVCluster] KV Server Leader elected");
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    spdlog::warn("[KVCluster] Timeout waiting for KV Server leader election!");
}

void KVCluster::StartAll() {
    for (auto &svr : kvservers_) svr->Start();
    for (auto &ck : clerks_) ck->Start();
}
void KVCluster::StopAll() {
    for (auto &svr : kvservers_) svr->Stop();
    for (auto &ck : clerks_) ck->Stop();
}

//------private methods------
}// namespace kv