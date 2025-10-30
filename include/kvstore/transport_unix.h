#pragma once 
#include "kvstore/transport.h"
#include "rpc/types.h"
#include <thread>       //for server thread and client thread
namespace kv{
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class KVTransportUnix 
    : public TransportBase,
      public IKVTransport,{
public:
    explicit KVTransportUnix(
        const rpc::type::PeerInfo&,
        const std::vector<rpc::type::PeerInfo>&);
    ~KVTransportUnix() override;
    
    void Start() override;
    void Stop() override;

    bool GetRPC(
        int,const GetArgs&,GetReply&) override;
    bool PutAppendRPC(
        int,const PutAppendArgs&,PutAppendReply&) override;
private:
    std::thread serverThread_;
    std::thread clientThread_;
};
}//namespace kv