#include "kvstore/transport.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace kv{
IKVTransport::IKVTransport(
    const rpc::type::PeerInfo& self,const std::vector<rpc::type::PeerInfo>& peers)
    : self_(self), peers_(peers) {}
void IKVTransport::RegisterGetHandler(
    std::function<std::string(const std::string&)> handler) {
    getHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("KV.Get", getHandler_);
    }
}
void IKVTransport::RegisterPutAppendHandler(
    std::function<std::string(const std::string&)> handler) {
    putAppendHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("KV.PutAppend", putAppendHandler_);
    }
}
}// namespace kv