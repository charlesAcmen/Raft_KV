#include "kvstore/transport_unix.h"
#include "rpc/client.h"
#include "rpc/server.h"
#include "kvstore/codec/kv_codec.h"
namespace kv{
KVTransportUnix::KVTransportUnix(
    const rpc::type::PeerInfo& self_,
    const std::vector<rpc::type::PeerInfo>& peer_)
    : TransportBase(self_, peer_) {}
KVTransportUnix::~KVTransportUnix() {
    Stop();
}
void KVTransportUnix::Start() {
    TransportBase::Start();
}
void KVTransportUnix::Stop() {
    TransportBase::Stop();
}
/**
 * @brief Synchronously call Get on target KV server.
 * 
 * @param targetId The peer node ID to which the RPC is sent
 * @param args Input arguments for the Get request
 * @param reply Output reply for the Get request
 * @return true if the RPC succeeded (response received), false on timeout/failure
 */
bool KVTransportUnix::GetRPC(
    int targetId,const type::GetArgs& args,type::GetReply& reply){
    return SendRPC<type::GetArgs,type::GetReply>(
        targetId,"KV.Get",args,reply,
        [](const type::GetArgs& a)->std::string{
            return codec::KVCodec::encode(a);
        },
        codec::KVCodec::decodeGetReply
    );
}
/**
 * @brief Synchronously call Put or Append on target KV server.
 * 
 * @param targetId The peer node ID to which the RPC is sent
 * @param args Input arguments for the Put/Append request
 * @param reply Output reply for the Put/Append request
 * @return true if the RPC succeeded, false on timeout/failure
 */
bool KVTransportUnix::PutAppendRPC(
    int targetId,const type::PutAppendArgs& args,type::PutAppendReply& reply){
    return SendRPC<type::PutAppendArgs,type::PutAppendReply>(
        targetId,"KV.PutAppend",args,reply,
        [](const type::PutAppendArgs& a)->std::string{
            return codec::KVCodec::encode(a);
        },
        codec::KVCodec::decodePutAppendReply
    );
}
void KVTransportUnix::RegisterGetHandler(
    rpc::type::RPCHandler handler) {
    getHandler_ = std::move(handler);
    TransportBase::RegisterHandler("KV.Get", getHandler_);
}
void KVTransportUnix::RegisterPutAppendHandler(
    rpc::type::RPCHandler handler) {
    putAppendHandler_ = std::move(handler);
    TransportBase::RegisterHandler("KV.PutAppend", putAppendHandler_);
}
}//namespace kv