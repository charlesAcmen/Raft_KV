#pragma once 
#include "rpc/transport.h"  //for rpc::ITransport
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
class IKVTransport: public rpc::ITransport{
public:
    virtual ~IKVTransport() = default;
    virtual bool GetRPC(int,
        const type::GetArgs&,
        type::GetReply&) = 0;
    virtual bool PutAppendRPC(int,
        const type::PutAppendArgs&,
        type::PutAppendReply&) = 0;
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