#pragma once 
#include "kvstore/types.h"
#include <string>
#include <sstream>
namespace kv::codec {
class KVCodec {
public:
    // ---------struct to string---------
    // GetArgs to string
    static inline std::string encode(const type::GetArgs& args) {
        std::stringstream ss;
        ss << args.Key << "\n";
        return ss.str();
    }
    static inline std::string encode(const type::GetReply& reply) {
        std::stringstream ss;
        ss << reply.err << "\n"
           << reply.Value << "\n";
        return ss.str();
    }
    // PutAppendArgs to string
    static inline std::string encode(const type::PutAppendArgs& args) {
        std::stringstream ss;
        ss << args.Key << "\n" 
           << args.Value << "\n" 
           << args.Op << "\n";
        return ss.str();
    }

    // ---------string to struct---------
    // string to GetReply
    static inline type::GetReply decodeGetReply(const std::string& payload) {
        struct type::GetReply reply{};
        std::stringstream ss(payload);
        std::string field;

        if(!std::getline(ss, field,'\n')) return {};
        if(field.empty()) return {};
        reply.Err = field;
        std::getline(ss, reply.err);
        std::getline(ss, reply.Value);  
        return reply;
    }
};// class KVCodec
}// namespace kv::codec