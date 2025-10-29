#include "kvstore/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
namespace kv{
KVTransportUnix::KVTransportUnix(
    const rpc::type::PeerInfo& self_,
    const std::vector<rpc::type::PeerInfo>& peer_)
    : IKVTransport(self_, peer_) {
    
}
KVTransportUnix::~KVTransportUnix() {
    
}
void KVTransportUnix::Start(){

}
void KVTransportUnix::Stop(){

}
bool KVTransportUnix::GetRPC(
    int targetId,const GetArgs& args,GetReply& reply){
    return false;
}
bool KVTransportUnix::PutAppendRPC(
    int targetId,const PutAppendArgs& args,PutAppendReply& reply){
    return false;
}   
}//namespace kv