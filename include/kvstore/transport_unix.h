#pragma once 
#include "kvstore/transport.h"
#include <chrono>       //for RPC timeout constexpr
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
protected:
    static constexpr std::chrono::milliseconds RPC_TIMEOUT{500};
};
}//namespace kv