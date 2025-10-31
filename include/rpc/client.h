#pragma once
#include "rpc/types.h"
#include <string>
#include <atomic>           //connected_ flag is atomic
#include <mutex>            //mutex for thread safety
#include <optional>         //for optional return type
/*
RPC message format is defined as:
[methodName]\n[payload]\nEND\n
*/
namespace rpc{
class RpcClient {
    public:
        RpcClient(const type::PeerInfo&,const type::PeerInfo&);
        ~RpcClient();
        bool Connect();
        void Close();
        //call rpc by method name and pass payload
        std::optional<std::string> Call(const std::string& method, const std::string& payload);
    private:
        //c++ 17+ supports inline
        inline static const int MAX_RETRIES = 5;
        inline static const int RETRY_INTERVAL_MS = 100;
        //fill struct addr and try connect to the host
        void initSocket();
        const type::PeerInfo selfInfo_;     //info of the client itself
        const type::PeerInfo targetInfo_;   //info of the rpc server

        std::atomic<bool> connected_{false};
        std::mutex conn_mtx_;                            
        int sock_fd{-1};
};//class RpcClient
}//namespace rpc