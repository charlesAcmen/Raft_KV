#pragma once
#include <string>
#include <optional> 
namespace rpc {
// a rpcrequest consists of a method name and a payload
struct RpcRequest {
    std::string method;
    std::string payload;
};
// a rpcresponse consists of a payload for now
struct RpcResponse{
    std::string payload;
};
// IMessageCodec: responsible for mutual conversion between RPC requests/responses and byte streams
// tryDecodeXXX accepts a growing buffer (std::string&):
// - if buffer contains a complete message, returns the parsed result and removes consumed bytes from buffer
// - if not,leave the buffer untouched and return std::nullopt
class IMessageCodec {
public:
    virtual ~IMessageCodec() = default;

    //encode a rpcrequest structor that contains method name and payload to a request string(framing)
    virtual std::string encodeRequest(const RpcRequest& req) = 0;
    //encode a payload string to a request string(framing)
    virtual std::string encodeRequest(const std::string& payload) = 0;
    //try to decode a request from buffer front
    virtual std::optional<RpcRequest> tryDecodeRequest(std::string& buffer) = 0;

    //encode a rpcresponse structor that contains payload to a response string(framing)
    virtual std::string encodeResponse(const RpcResponse& payload) = 0;
    //encode a payload string to a response string(framing)
    virtual std::string encodeResponse(const std::string& payload) = 0;
    //try to decode a response from buffer front
    virtual std::optional<std::string> tryDecodeResponse(std::string& buffer) = 0;
};//class IMessageCodec
} // namespace rpc
/*
why base class is called IMessageCodec?
I:interface,naming convention inherited from COM(Compoent Object Model)
meaning the class is a abstract interface class,i.e. pure virtual functions only
*/