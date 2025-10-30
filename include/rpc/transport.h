#pragma once
#include "rpc/types.h"  //for PeerInfo
#include <vector>       //for list of peers
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
#include <thread>       //for server and client threads
namespace rpc{
class RpcClient;
class RpcServer;
class TransportBase{
public:
    virtual ~TransportBase() = default;

    virtual void Start();
    virtual void Stop();
protected:
    TransportBase(
        const type::PeerInfo& self,
        const std::vector<type::PeerInfo>& peers);
    // Information about self and peers
    const type::PeerInfo self_;
    const std::vector<type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<RpcClient>> clients_;

    std::thread serverThread_;
    std::thread clientThread_;
};
}