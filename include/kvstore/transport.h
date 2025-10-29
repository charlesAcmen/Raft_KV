#pragma once 
#include "rpc/transport.h"
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
namespace rpc {
    class RpcClient;
    class RpcServer;
}

namespace kv{
class IKVTransport : public rpc::ITransport {
public:
    virtual ~IKVTransport() = default;
protected:

    // RPC server and clients
    std::unique_ptr<rpc::RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;
};
}//namespace kv