#include "rpc/transport.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace rpc{
ITransport::ITransport(
    const type::PeerInfo& self,
    const std::vector<type::PeerInfo>& peers)
    : self_(self), peers_(peers) {
    // Start RPC server
    server_ = std::make_unique<rpc::RpcServer>(self_);
    
    // Create RPC clients for each peer
    for (const auto& peer : peers_) {
        if (peer.id == self_.id) continue; // Skip self
        clients_[peer.id] = std::make_unique<rpc::RpcClient>(self_,peer);
    }
    // do not start the server here, because the handlers are not registered yet
    // and node will be blocked when starting the server
}
}// namespace rpc