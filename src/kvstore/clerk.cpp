#include "kvstore/clerk.h"
namespace kv {
std::string Clerk::Get(const std::string& key) {
    return ""; // TODO: implement
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
    //TODO:          
}
}//namespace kv