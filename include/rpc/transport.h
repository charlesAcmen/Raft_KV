#pragma once
#include "rpc/types.h"  //for PeerInfo
#include <vector>       //for list of peers
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
#include <thread>       //for server and client threads
#include <functional>   //for encode and decode functions
namespace rpc{
class RpcClient;
class RpcServer;
class TransportBase{
public:
    virtual ~TransportBase() = default;

    void Start();
    void Stop();
protected:
    TransportBase(
        const type::PeerInfo& self,
        const std::vector<type::PeerInfo>& peers);


    void RegisterHandler(
        const std::string& rpcName,
        const rpc::type::RPCHandler& handler);

    template<typename Args,typename Reply>
    void SendRPC(
        int targetID,const std::string& rpcName, 
        const Args&,Reply&,
        const std::function<std::string(const Args&)>&,
        const std::function<bool(const std::string&, Reply&)>&);

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