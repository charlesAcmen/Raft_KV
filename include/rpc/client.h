#pragma once
#include <string>

/*
RPC message format is defined as:
[methodName]\n[payload]\nEND\n
*/

namespace rpc{
    class RpcClient {
        public:
            inline static const std::string SOCK_PATH = "/tmp/mr-rpc.sock";
            
            RpcClient();
            ~RpcClient();
            //call rpc by method name and pass payload
            std::string call(const std::string& method, const std::string& payload);
        private:

            //fill struct addr and try connect to the host
            void initSocket();

            //c++ 17+ supports inline
            inline static const int MAX_RETRIES = 5;
            inline static const int RETRY_INTERVAL_MS = 100;

            //accept() will return a new socket fd called client_fd for communication
            //it is the socket that serves to send and receive data
            //sock_fd is the socket fd used to connect to the server
            int sock_fd{-1};
    };
}//namespace rpc