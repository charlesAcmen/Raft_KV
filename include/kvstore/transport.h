#pragma once 
#include "rpc/transport.h"
#include "kvstore/types.h"  //for GetArgs, GetReply, PutAppendArgs, PutAppendReply
namespace kv{
/**
 * @brief Abstract transport layer for KVServer RPCs.
 * 
 * This interface defines synchronous(同步) RPC calls for a KV server cluster,
 * such as Get and PutAppend, as well as registration of RPC handlers.
 * It inherits from rpc::ITransport, which provides Start/Stop lifecycle
 * control for the transport.
 */
class IKVTransport{
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
        int targetId,const PutAppendArgs& args,PutAppendReply& reply) = 0;

    virtual void RegisterGetHandler(
        rpc::type::RPCHandler handler) = 0;
    virtual void RegisterPutAppendHandler(
        rpc::type::RPCHandler handler) = 0;
protected:
    // RPC handlers
    rpc::type::RPCHandler getHandler_;
    rpc::type::RPCHandler putAppendHandler_;
};
}//namespace kv