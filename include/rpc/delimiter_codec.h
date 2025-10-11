#pragma once
#include "rpc/message_codec.h"
#include <memory>

namespace rpc {

    class DelimiterCodec : public IMessageCodec {
        public:
            DelimiterCodec(std::string delimiter = "\nEND\n");

            std::string encodeRequest(const RpcRequest& req) override;
            std::string encodeRequest(const std::string& payload) override;
            std::optional<RpcRequest> tryDecodeRequest(std::string& buffer) override;

            std::string encodeResponse(const std::string& payload) override;
            std::string encodeResponse(const RpcResponse& payload) override;
            std::optional<std::string> tryDecodeResponse(std::string& buffer) override;

        private:
            // delimiter used to frame messages
            const std::string delim;
    };

} // namespace rpc
