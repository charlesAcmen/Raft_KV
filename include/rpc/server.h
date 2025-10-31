#pragma once

#include "rpc/types.h"
#include <string>
#include <functional>   //handler function
#include <unordered_map> //handlers
#include <atomic>   //server running flag
#include <vector>  //clientThreads_
#include <thread>  //clientThreads_
#include <mutex>   //protect clientThreads_
/*
RPC response format is defined as:
[response payload]\nEND\n
*/



namespace rpc{
class RpcServer {
    public:
        RpcServer(const type::PeerInfo& selfInfo);
        ~RpcServer();
        //register rpc handler by method name
        void Register_Handler(const std::string& method,type::RPCHandler handler);
        void Start();
        void Stop();
    private:
        void initSocket();
        void handleClient(int client_fd);

        //key: method name, value: handler function
        std::unordered_map<std::string, type::RPCHandler> handlers_;
        std::vector<std::thread> clientThreads_;
        //mutex to protect clientThreads_ when adding new threads
        std::mutex threads_mtx_;
        type::PeerInfo selfInfo_;

        int server_fd{-1};
        //server running flag
        std::atomic<bool> running{true};
};
}//namespace rpc
