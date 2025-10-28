#pragma once
#include <string>
#include <vector>
namespace kv {
class Clerk {
public:
    Clerk() = default;

    std::string Get(const std::string& key) const;
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& arg);

private:
    void PutAppend(
        const std::string& key, const std::string& value, const std::string op);
};

}//namespace kv