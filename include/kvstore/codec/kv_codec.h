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
        ss << type::ErrToString(reply.err) << "\n"
           << reply.Value << "\n";
        return ss.str();
    }
    // PutAppendArgs to string
    static inline std::string encode(const type::PutAppendArgs& args) {
        std::stringstream ss;
        ss << args.Key << "\n" 
           << args.Value << "\n" 
           << args.Op << "\n"
           << args.ClientId << "\n"
           << args.RequestId << "\n";
        return ss.str();
    }
    static inline std::string encode(const type::PutAppendReply& reply) {
        std::stringstream ss;
        ss << type::ErrToString(reply.err) << "\n";
        return ss.str();
    }


    // ---------string to struct---------
    // string to PutAppendArgs
    static inline type::PutAppendArgs decodePutAppendArgs(const std::string& payload) {
        struct type::PutAppendArgs args{};
        std::stringstream ss(payload);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        args.Key = field;
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        args.Value = field;
        if (!std::getline(ss, field, '\n')) return {}; 
        if( field.empty()) return {};
        args.Op = field; 
        if (!std::getline(ss, field, '\n')) return {}; 
        if( field.empty()) return {};
        try {
            args.ClientId = std::stoi(field);
        } catch (...) {
            args.ClientId = -1;
        }
        if (!std::getline(ss, field, '\n')) return {}; 
        if( field.empty()) return {};
        try {
            args.RequestId = std::stoi(field);
        } catch (...) {
            args.RequestId = -1;
        }
        return args;
    }
    // string to PutAppendReply
    static inline type::PutAppendReply decodePutAppendReply(const std::string& payload) {
        struct type::PutAppendReply reply{};
        std::stringstream ss(payload);
        std::string field;
        if(!std::getline(ss, field,'\n')) return {};
        if(field.empty()) return {};    
        reply.err = type::StringToErr(field);
        return reply;
    }
    // string to GetArgs
    static inline type::GetArgs decodeGetArgs(const std::string& payload) {
        struct type::GetArgs args{};
        std::stringstream ss(payload);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if( field.empty()) return {};
        args.Key = field;
        return args;
    }
    // string to GetReply
    static inline type::GetReply decodeGetReply(const std::string& payload) {
        struct type::GetReply reply{};
        std::stringstream ss(payload);
        std::string field;

        if(!std::getline(ss, field,'\n')) return {};
        if(field.empty()) return {};
        reply.err = type::StringToErr(field);
        if(!std::getline(ss, field,'\n')) return {};
        if(field.empty()) return {};
        reply.Value = field;
        return reply;
    }
};// class KVCodec
}// namespace kv::codec