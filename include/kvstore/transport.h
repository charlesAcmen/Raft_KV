#pragma once 
#include "rpc/transport.h"
namespace kv{
class IKVTransport : public rpc::ITransport {
public:
    virtual ~IKVTransport() = default;
protected:

};
}//namespace kv