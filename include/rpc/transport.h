#pragma once
#include "rpc/types.h"
#include <vector>       //for list of peers
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
namespace rpc{
class RpcClient;
class RpcServer;
class ITransport{
public:
    virtual ~ITransport() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
protected:
    ITransport(
        const type::PeerInfo& self,
        const std::vector<type::PeerInfo>& peers);
    // Information about self and peers
    const type::PeerInfo self_;
    const std::vector<type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<RpcClient>> clients_;
};
}