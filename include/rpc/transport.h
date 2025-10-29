#pragma once
namespace rpc{
class ITransport {
public:
    virtual ~ITransport() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
};
}