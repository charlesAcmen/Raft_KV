#pragma once
#include <string>
#include <sstream>
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
    enum class CommandType {PUT,APPEND,GET,INVALID};
    inline static std::string CommandType2String(CommandType type) {
        switch(type) {
            case CommandType::PUT: return "Put";
            case CommandType::APPEND: return "Append";
            case CommandType::GET: return "Get";
            default: return "Invalid";
        }
    }
    inline static CommandType String2CommandType(const std::string& str) {
        if (str == "Put") return CommandType::PUT;
        if (str == "Append") return CommandType::APPEND;
        if (str == "Get") return CommandType::GET;
        return CommandType::INVALID;
    }

    CommandType type;
    std::string key;
    std::string value;

    KVCommand(
        CommandType t = CommandType::INVALID, const std::string& k = "", const std::string& v = "")
        : type(t), key(k), value(v) {}

    std::string ToString() const {
        return CommandType2String(type) + "\n" + key + "\n" + value + "\n";
    }
    static inline KVCommand FromString(const std::string& str) {
        struct KVCommand command{};
        std::stringstream ss(str);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        command.type = String2CommandType(field);
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        command.key = field;
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        command.value = field;
        return command;
    }
};//struct KVCommand
} // namespace kv::type
