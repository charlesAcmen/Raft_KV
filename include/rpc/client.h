#pragma once
#include "raft/types.h"
#include <string>
#include <atomic>           //connected_ flag is atomic
#include <mutex>            //mutex for thread safety
/*
RPC message format is defined as:
[methodName]\n[payload]\nEND\n
*/

namespace rpc{
    class RpcClient {
        public:
            RpcClient(const raft::type::PeerInfo& selfInfo,
                    const raft::type::PeerInfo& targetInfo);
            ~RpcClient();
            bool Connect();
            void Close();
            //call rpc by method name and pass payload
            std::string Call(const std::string& method, const std::string& payload);
        private:

            //fill struct addr and try connect to the host
            void initSocket();

            //c++ 17+ supports inline
            inline static const int MAX_RETRIES = 5;
            inline static const int RETRY_INTERVAL_MS = 100;

            const raft::type::PeerInfo selfInfo_;     //info of the client itself
            const raft::type::PeerInfo targetInfo_;   //info of the rpc server

            std::atomic<bool> connected_{false};
            std::mutex conn_mtx_;                            
            int sock_fd{-1};
    };
}//namespace rpc