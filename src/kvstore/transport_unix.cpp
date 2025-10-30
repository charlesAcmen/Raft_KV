#include "kvstore/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace kv{
KVTransportUnix::KVTransportUnix(
    const rpc::type::PeerInfo& self_,
    const std::vector<rpc::type::PeerInfo>& peer_)
    : TransportBase(self_, peer_) {
    
}
bool KVTransportUnix::GetRPC(
    int targetId,const GetArgs& args,GetReply& reply){
    return false;
}
bool KVTransportUnix::PutAppendRPC(
    int targetId,const PutAppendArgs& args,PutAppendReply& reply){
    return false;
}   


void KVTransportUnix::RegisterGetHandler(
    rpc::type::RPCHandler handler) {
    getHandler_ = std::move(handler);
    if (TransportBase::server_) {
        server_->Register_Handler("KV.Get", getHandler_);
    }
}
void KVTransportUnix::RegisterPutAppendHandler(
    rpc::type::RPCHandler handler) {
    putAppendHandler_ = std::move(handler);
    if (server_) {
        server_->Register_Handler("KV.PutAppend", putAppendHandler_);
    }
}
}//namespace kv