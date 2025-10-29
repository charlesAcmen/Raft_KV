#pragma once 
#include "kvstore/transport.h"
#include "rpc/types.h"
namespace kv{
// Unix-domain-socket-based transport for single-machine multi-process simulation.
class KVTransportUnix : public IKVTransport {
public:
    explicit KVTransportUnix(
        const rpc::type::PeerInfo&,
        const std::vector<rpc::type::PeerInfo>&);
    ~KVTransportUnix() override;
    
    void Start() override;
    void Stop() override;
private:
};
}//namespace kv