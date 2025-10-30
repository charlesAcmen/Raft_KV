#pragma once 
#include "rpc/transport.h"
#include "kvstore/transport.h"
#include "rpc/types.h"
namespace kv{
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class KVTransportUnix 
    : public rpc::TransportBase,
      public IKVTransport{
public:
    explicit KVTransportUnix(
        const rpc::type::PeerInfo&,
        const std::vector<rpc::type::PeerInfo>&);
    ~KVTransportUnix() override;

    virtual void Start() override;
    virtual void Stop() override;

    bool GetRPC(
        int,const GetArgs&,GetReply&) override;
    bool PutAppendRPC(
        int,const PutAppendArgs&,PutAppendReply&) override;
    virtual void RegisterGetHandler(
        rpc::type::RPCHandler handler) override;
    virtual void RegisterPutAppendHandler(
        rpc::type::RPCHandler handler) override;
};
}//namespace kv