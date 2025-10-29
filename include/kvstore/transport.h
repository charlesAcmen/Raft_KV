#pragma once 
#include "rpc/transport.h"
#include "kvstore/types.h"  //for GetArgs, GetReply, PutAppendArgs, PutAppendReply
#include <unordered_map>//for map of clients
#include <memory>       //for unique_ptr
#include <functional>   //for rpc handlers
#include <vector>       //for list of peers
#include <string>
namespace rpc {
    class RpcClient;
    class RpcServer;
}

namespace kv{
/**
 * @brief Abstract transport layer for KVServer RPCs.
 * 
 * This interface defines synchronous(同步) RPC calls for a KV server cluster,
 * such as Get and PutAppend, as well as registration of RPC handlers.
 * It inherits from rpc::ITransport, which provides Start/Stop lifecycle
 * control for the transport.
 */
class IKVTransport : public rpc::ITransport {
public:
    virtual ~IKVTransport() = default;
    /**
     * @brief Synchronously call Get on target KV server.
     * 
     * @param targetId The peer node ID to which the RPC is sent
     * @param args Input arguments for the Get request
     * @param reply Output reply for the Get request
     * @return true if the RPC succeeded (response received), false on timeout/failure
     */
    virtual bool GetRPC(
        int targetId,const GetArgs& args,GetReply& reply) = 0;

    /**
     * @brief Synchronously call Put or Append on target KV server.
     * 
     * @param targetId The peer node ID to which the RPC is sent
     * @param args Input arguments for the Put/Append request
     * @param reply Output reply for the Put/Append request
     * @return true if the RPC succeeded, false on timeout/failure
     */
    virtual bool PutAppendRPC(
        int targetId, 
                              const PutAppendArgs& args, 
                              PutAppendReply& reply) = 0;

    /**
     * @brief Register handler function for Get RPCs.
     * 
     * @param handler Function that receives serialized input and returns serialized output
     */
    virtual void RegisterGetHandler(
        std::function<std::string(const std::string&)> handler);

    /**
     * @brief Register handler function for PutAppend RPCs.
     * 
     * @param handler Function that receives serialized input and returns serialized output
     */
    virtual void RegisterPutAppendHandler(
        std::function<std::string(const std::string&)> handler);
protected:
    /**
     * @brief Protected constructor to be called by derived classes.
     * 
     * @param self Information about this KV server (id + address)
     * @param peers Information about all peers in the cluster
     */
    IKVTransport(
        const rpc::type::PeerInfo& self,
        const std::vector<rpc::type::PeerInfo>& peers);

    // Information about self and all peers
    const rpc::type::PeerInfo self_;
    const std::vector<rpc::type::PeerInfo> peers_;

    // RPC server and clients
    std::unique_ptr<rpc::RpcServer> server_;
    // key: peer id
    std::unordered_map<int, std::unique_ptr<rpc::RpcClient>> clients_;

    // RPC handlers
    std::function<std::string(const std::string&)> getHandler_;
    std::function<std::string(const std::string&)> putAppendHandler_;
};
}//namespace kv