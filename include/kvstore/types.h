#pragma once
#include <string>
namespace kv::type{
//better than typedef
// using Err = std::string;
enum class Err {
    OK,
    ErrNoKey,
    ErrWrongLeader,
    ErrTimeout,
    ErrUnknown
};
inline std::string ErrToString(Err err) {
    switch (err) {
        case Err::OK: return "OK";
        case Err::ErrNoKey: return "ErrNoKey";
        case Err::ErrWrongLeader: return "ErrWrongLeader";
        case Err::ErrTimeout: return "ErrTimeout";
        case Err::ErrUnknown: return "ErrUnknown";
        default: return "InvalidErr";
    }
};
inline Err StringToErr(const std::string& str) {
    if (str == "OK") return Err::OK;
    if (str == "ErrNoKey") return Err::ErrNoKey;
    if (str == "ErrWrongLeader") return Err::ErrWrongLeader;
    if (str == "ErrTimeout") return Err::ErrTimeout;
    if (str == "ErrUnknown") return Err::ErrUnknown;
    return Err::ErrUnknown; // default case
}
struct PutAppendArgs{
    std::string Key;
    std::string Value;
    std::string Op; // "Put" or "Append"
    // You'll have to add definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
    int ClientId;
    int RequestId;
};
struct PutAppendReply{
    Err err;
};
struct GetArgs{
    std::string Key;
    // You'll have to add definitions here.
    int ClientId;
    int RequestId;
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
} // namespace kv::codec
