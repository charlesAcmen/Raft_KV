#pragma once 
#include <string>
#include <functional>
namespace rpc::type {
using PeerID = int;
// RPC handler
// accept a serialized payload and return a serialized reply.
using RPCHandler = std::function<std::string(const std::string&)>;
// for now only support Unix domain socket transport
// In real world, we may want to support TCP/IP as well
struct PeerInfo {
    PeerID id;             // unique rpc node ID
    std::string address;   // /tmp/raft-node-<id>.sock,Unix socket path for RPC
};
}//namespace rpc