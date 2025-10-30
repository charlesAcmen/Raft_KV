#include "kvstore/clerk.h"
namespace kv {
Clerk::Clerk(int me,const std::vector<int>& peers,
std::shared_ptr<IKVTransport> transport)
    :me_(me),peers_(peers),transport_(transport){

}
std::string Clerk::Get(const std::string& key) const {
    return "";
}
void Clerk::Put(const std::string& key, const std::string& value) {
    PutAppend(key, value, "Put");
}
void Clerk::Append(const std::string& key, const std::string& arg) {
    PutAppend(key, arg, "Append");
}
void Clerk::PutAppend(
    const std::string& key, 
    const std::string& value,
    const std::string op) { 
          
}
}//namespace kv