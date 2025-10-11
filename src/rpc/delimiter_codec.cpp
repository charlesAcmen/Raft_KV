#include "rpc/delimiter_codec.h"
#include <sstream>
#include <memory>
#include <spdlog/spdlog.h>
// protocol: 
// RpcRequest:[method]\n[payload]\nEND\n
// RpcResponse:[payload]\nEND\n
namespace rpc {
    DelimiterCodec::DelimiterCodec(std::string delimiter)
        :delim(std::move(delimiter)) {}

    std::string DelimiterCodec::encodeRequest(const RpcRequest& req){
        std::string out;
        //'\n' is 1 byte
        out.reserve(req.method.size() + req.payload.size() + delim.size() + 1);
        out += req.method;
        out += '\n';
        out += req.payload;
        out += delim;
        return out;
    }
    std::string DelimiterCodec::encodeRequest(const std::string& payload){
        std::string out;
        //'\n' is 1 byte
        out.reserve(payload.size() + delim.size() + 1);
        out += payload;
        out += delim;
        return out;
    }

    std::optional<RpcRequest> DelimiterCodec::tryDecodeRequest(std::string& buffer){
        size_t pos = buffer.find(delim);
        if (pos == std::string::npos){
            // no complete message yet
            return std::nullopt;
        }
        // found a delimiter, extract the message
        std::string msg = buffer.substr(0, pos);
        // remove the processed message from buffer including the delimiter
        buffer.erase(0, pos + delim.size());

        std::istringstream iss(msg);
        // 1️. resolve method
        std::string method;
        if (!std::getline(iss, method)) {
            spdlog::error("[Delimiter_codec] delimiter_codec::tryDecodeRequest Failed to parse method from RPC request: {}", msg);
            return std::nullopt;
        }
        // 2️. resolve payload
        std::string payload;
        // if there is more data, read until '\0'
        // otherwise, payload is empty
        if (iss.peek() != EOF) {
            std::getline(iss, payload, '\0');
        } else {
            payload = "";
        }
        //payload is the rest of the message until '\0'
        //from c++ 11 onwards,string data will be null-terminated
        //getline will stop at '\n' by default,so specify '\0' as the delimiter
        //and it may stop at the end of the stream if there is no '\0'

        return RpcRequest{method, payload};
    }

    std::string DelimiterCodec::encodeResponse(const std::string& payload){
        return payload + delim;
    }
    std::string DelimiterCodec::encodeResponse(const RpcResponse& payload){
        return payload.payload + delim;
    }

    std::optional<std::string> DelimiterCodec::tryDecodeResponse(std::string& buffer) {
        size_t pos = buffer.find(delim);
        if (pos == std::string::npos) {
            // no complete message yet
            return std::nullopt;
        }
        std::string resp = buffer.substr(0, pos);
        buffer.erase(0, pos + delim.size());
        return resp;
    }
} // namespace rpc
