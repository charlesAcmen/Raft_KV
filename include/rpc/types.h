#pragma once 
#include <string>
namespace rpc::type {
// for now only support Unix domain socket transport
// In real world, we may want to support TCP/IP as well
struct PeerInfo {
    int id;                 // unique rpc node ID
    std::string address;   // /tmp/raft-node-<id>.sock,Unix socket path for RPC
};
}//namespace rpc