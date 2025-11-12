#pragma once
#include "rpc/types.h"  //for PeerInfo
#include "rpc/client.h"
#include "rpc/server.h"
#include <atomic>       //for atomic running flag
#include <vector>       //for list of peers
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
#include <thread>       //for server and client threads
#include <functional>   //for encode and decode functions
#include <spdlog/spdlog.h>
namespace rpc{
class TransportBase{
public:
    virtual ~TransportBase() = default;
    //not virtual to avoid vtable ambiguity in multiple inheritance
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
    bool SendRPC(
        int targetId,const std::string& rpcName, 
        const Args& args,Reply& reply,
        const std::function<std::string(const Args&)>& encodeFn,
        const std::function<Reply(const std::string&)>& decodeFn){
        auto it = clients_.find(targetId);
        if (it == clients_.end()) {
            spdlog::error("[TransportBase] No RPC client for peer {}", targetId);
            return false;
        }
        rpc::RpcClient& client = *it->second;//unique_ptr

        //convert args to string as request
        std::string request = encodeFn(args);
        spdlog::info("[TransportBase] RPC {} to peer {} finished encode",rpcName, targetId);
        std::optional<std::string> response = client.Call(rpcName, request);
        spdlog::info("[TransportBase] RPC {} to peer {} finished Call", rpcName, targetId);
        if(!response) return false;
        spdlog::info("[TransportBase] RPC {} to peer {} completed", rpcName, targetId);
        reply = decodeFn(*response);
        return true;
    }

    // Information about self and peers
    const type::PeerInfo self_;
    const std::vector<type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<RpcClient>> clients_;

    std::thread serverThread_;
    std::thread clientThread_;

    std::atomic<bool> running_{false};
};//class TransportBase
class ITransport{
public:
    virtual ~ITransport() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
};//class ITransport
}//namespace rpc