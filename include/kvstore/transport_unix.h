#pragma once 
#include "rpc/transport.h"  // for rpc::TransportBase
#include "kvstore/transport.h"  // for IKVTransport
namespace kv {
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class KVTransportUnix 
    : public rpc::TransportBase,
      public IKVTransport {
public:
    explicit KVTransportUnix(
        const rpc::type::PeerInfo&,
        const std::vector<rpc::type::PeerInfo>&);
    ~KVTransportUnix() override;

    virtual void Start() override;
    virtual void Stop() override;

    bool GetRPC(int,
        const type::GetArgs&,
        type::GetReply&) override;
    bool PutAppendRPC(int,
        const type::PutAppendArgs&,
        type::PutAppendReply&) override;
    virtual void RegisterGetHandler(
        rpc::type::RPCHandler handler) override;
    virtual void RegisterPutAppendHandler(
        rpc::type::RPCHandler handler) override;
};
}//namespace kv