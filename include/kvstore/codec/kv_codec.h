#pragma once 
#include "kvstore/types.h"
#include <string>
#include <sstream>
namespace kv::codec {
class KVCodec {
public:
    //---------struct to string---------
    //PutAppendArgs to string
    static inline std::string encode(const type::PutAppendArgs& args) {
        std::stringstream ss;
        ss << args.key << "\n" << args.value;
        return ss.str();
    }

    //GetArgs to string
    static inline std::string encode(const type::GetArgs& args) {
        std::stringstream ss;
        ss << args.key;
        return ss.str();
    }
};// class KVCodec
}// namespace kvstore::codec