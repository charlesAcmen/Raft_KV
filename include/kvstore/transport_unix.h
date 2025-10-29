#pragma once 
#include "kvstore/transport.h"
namespace kv{
class KVTransportUnix : public IKVTransport {
public:
    virtual ~KVTransportUnix() = default;
    
    void Start() override;
    void Stop() override;
protected:
};
}//namespace kv