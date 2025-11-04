#include "rpc/transport.h"
namespace rpc{
TransportBase::TransportBase(
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
void TransportBase::Start() {
    if(running_.exchange(true)) return;//already started.
    // Start the server in background
    serverThread_ = std::thread([this]() { server_->Start(); });

    clientThread_ = std::thread([this]() {
        while(running_.load()){
            bool allConnected = true;
            for (auto& [id, client] : clients_) {
                if(client->Connect()){
                    // spdlog::info("[TransportBase] {} Connected to peer {}", this->self_.id, id);
                }else{
                    allConnected = false;
                }
            }
            if(allConnected || !running_.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        spdlog::info("[TransportBase] {}:All RPC clients connected to peers",self_.address);
    });
}
void TransportBase::Stop() {
    if(!running_.exchange(false)) return;//already stopped
    if (server_) server_->Stop();
    for (auto& [id, client] : clients_) { client->Close();}
    if (serverThread_.joinable()) serverThread_.join();
    if (clientThread_.joinable()) clientThread_.join();
}
void TransportBase::RegisterHandler(
    const std::string& rpcName,
    const rpc::type::RPCHandler& handler) {
    if (server_) {server_->Register_Handler(rpcName, handler);} 
    else {spdlog::error("[TransportBase] RegisterHandler failed: server not initialized");}
}
}// namespace rpc