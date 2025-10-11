#pragma once
#include "raft/types.h"
#include <string>

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
            //call rpc by method name and pass payload
            std::string call(const std::string& method, const std::string& payload);
        private:

            //fill struct addr and try connect to the host
            void initSocket();

            //c++ 17+ supports inline
            inline static const int MAX_RETRIES = 5;
            inline static const int RETRY_INTERVAL_MS = 100;

            raft::type::PeerInfo selfInfo_;     //info of the client itself
            raft::type::PeerInfo targetInfo_;   //info of the rpc server


            //accept() will return a new socket fd called client_fd for communication
            //it is the socket that serves to send and receive data
            //sock_fd is the socket fd used to connect to the server
            int sock_fd{-1};
    };
}//namespace rpc