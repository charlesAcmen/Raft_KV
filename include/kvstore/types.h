#pragma once
#include <string>
namespace kv{
//better than typedef
using Err = std::string;
struct PutAppendArgs{
    std::string Key;
    std::string Value;
    std::string Op; // "Put" or "Append"
    // You'll have to add definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
};
struct PutAppendReply{
    Err Err;
};
struct GetArgs{
    std::string Key;
    // You'll have to add definitions here.
};
struct GetReply{
    Err err;
    std::string Value;
};

struct KVCommand {
    enum class CommandType {PUT,APPEND,GET};

    CommandType type;
    std::string key;
    std::string value;

    KVCommand(CommandType t, const std::string& k, const std::string& v = "")
        : type(t), key(k), value(v) {}
};
} // namespace kv
