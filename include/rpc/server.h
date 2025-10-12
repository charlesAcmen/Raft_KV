#pragma once

#include "raft/types.h"

#include <string>
#include <functional>   //handler function
#include <unordered_map> //handlers
#include <atomic>   //server running flag

/*
RPC response format is defined as:
[response payload]\nEND\n
*/



namespace rpc{
class RpcServer {
    public:
        RpcServer(const raft::type::PeerInfo& selfInfo);
        ~RpcServer();
        //register rpc handler by method name
        void register_handler(const std::string& method,std::function<std::string(const std::string&)> handler);
        void start();
        void stop();
    private:
        void initSocket();
        void handleClient(int client_fd);

        //return type : std::string, param type: const std::string& as payload
        std::unordered_map<std::string, std::function<std::string(const std::string&)>> handlers;

        //do not use thread pool here, because each rpc client has long connection with rpc server
        //use detached thread instead

        raft::type::PeerInfo selfInfo_;

        int server_fd{-1};
        //server running flag
        std::atomic<bool> running{true};
};
}//namespace rpc
