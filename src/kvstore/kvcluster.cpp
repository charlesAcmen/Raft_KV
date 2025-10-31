#include "kvstore/kvcluster.h"
#include "kvstore/kvserver.h"
#include "kvstore/clerk.h"
// #include "raft/raft.h"                
#include "rpc/types.h"              //rpc::type::PeerInfo
#include "kvstore/transport_unix.h"      //IKVTransport and KVTransportUnix
#include "raft/transport_unix.h"   //IRaftTransport and RaftTransportUnix
namespace kv{
KVCluster::KVCluster(int numServers, int numClerks) {
    //preparing peer info
    std::vector<rpc::type::PeerInfo> peers;
    std::vector<int> peerIds;
    int numNodes = numServers*numClerks;
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
    std::vector<std::shared_ptr<raft::Raft>> raftNodes = createRaftNodes(numServers);
    for (int i = numClerks; i < numNodes; ++i) {
        rpc::type::PeerInfo self = peers[i];
        std::shared_ptr<IKVTransport> transport = 
            std::make_shared<KVTransportUnix>(self, peers);
        // inject transport into KV server and create server
        std::shared_ptr<KVServer> kvserver = 
            std::make_shared<KVServer>(self.id, peerIds, transport, raftNodes[i-numClerks], -1);        
        kvservers_.push_back(kvserver);
    }
}
KVCluster::~KVCluster() {
    StopAll();
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
std::vector<std::shared_ptr<raft::Raft>> KVCluster::createRaftNodes(int numNodes) {
    //preparing peer info
    std::vector<rpc::type::PeerInfo> peers;
    std::vector<int> peerIds;
    for (int i = 0; i < numNodes; ++i) {
        peers.push_back({i+1, "/tmp/raft-node-" + std::to_string(i+1) + ".sock"});
        peerIds.push_back(i+1);
    }
    std::vector<std::shared_ptr<raft::Raft>> nodes;
    for (int i = 0; i < numNodes; ++i) {
        rpc::type::PeerInfo self = peers[i];
        std::shared_ptr<raft::IRaftTransport> transport = 
            std::make_shared<raft::RaftTransportUnix>(self, peers);
        // inject transport into Raft node and create node
        std::shared_ptr<raft::Raft> raftNode = 
            std::make_shared<raft::Raft>(self.id, peerIds,transport);        
        nodes.push_back(raftNode);
    }
    return nodes;
}

}// namespace kv