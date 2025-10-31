#pragma once
#include "kvstore/transport.h"
#include <string>
#include <vector>
#include <memory>   //shared_ptr
namespace kv {
class Clerk {
public:
    Clerk(int,const std::vector<int>&,std::shared_ptr<IKVTransport>);
    ~Clerk();
    void Start();
    void Stop();

    std::string Get(const std::string& key) const;
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& arg);

private:
    void PutAppend(
        const std::string& key, const std::string& value, const std::string op);

    const int me_;                      // this peer's id (index into peers_)
    const std::vector<int> peers_;      // peer ids (including me_)
    std::shared_ptr<IKVTransport> transport_;
};//class Clerk
}//namespace kv