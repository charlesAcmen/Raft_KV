#pragma once
namespace kv{
struct KVCommand {
    enum class CommandType {PUT,APPEND,GET};

    CommandType type;
    std::string key;
    std::string value;

    KVCommand(CommandType t, const std::string& k, const std::string& v = "")
        : type(t), key(k), value(v) {}
};
} // namespace kv
