#pragma once
#include "../types.h"
#include <string>
#include <sstream>

namespace raft::codec {

class RaftCodec {
public:
    //RequestVoteArgs to string
    static inline std::string encode(const type::RequestVoteArgs& args) {
        std::stringstream ss;
        ss << args.term << "\n" << args.candidateId
           << "\n" << args.lastLogIndex << "\n" << args.lastLogTerm;
        return ss.str();
    }

    //AppendEntriesArgs to string
    static inline std::string encode(const type::AppendEntriesArgs& args) {
        std::stringstream ss;
        ss << args.term << "\n" << args.leaderId << "\n" << args.prevLogIndex
           << "\n" << args.prevLogTerm << "\n" << args.entries.size()
           << "\n" << args.leaderCommit;
        return ss.str();
    }

    //RequestVoteReply to string
    static inline std::string encode(const type::RequestVoteReply& reply) {
        std::stringstream ss;
        ss << reply.term << "\n" << (reply.voteGranted ? "1" : "0");
        return ss.str();
    }

    //AppendEntriesReply to string
    static inline std::string encode(const type::AppendEntriesReply& reply) {
        std::stringstream ss;
        ss << reply.term << "\n" << (reply.success ? "1" : "0");
        return ss.str();
    }


    //string to RequestVoteArgs
    static inline type::RequestVoteArgs decodeRequestVoteArgs(const std::string& payload) {
        struct type::RequestVoteArgs args{};
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, '\n'); args.term = std::stoi(field);
        std::getline(ss, field, '\n'); args.candidateId = std::stoi(field);
        std::getline(ss, field, '\n'); args.lastLogIndex = std::stoi(field);
        std::getline(ss, field, '\n'); args.lastLogTerm = std::stoi(field);
        return args;
    }

    

    //string to AppendEntriesArgs
    static inline type::AppendEntriesReply decodeAppendEntries(const std::string& payload) {
        struct type::AppendEntriesReply args{};
        // std::stringstream ss(payload);
        // std::string field;
        // std::getline(ss, field, '\n'); args.term = std::stoi(field);
        // std::getline(ss, field, '\n'); args.leaderId = std::stoi(field);
        // std::getline(ss, field, '\n'); args.prevLogIndex = std::stoi(field);
        // std::getline(ss, field, '\n'); args.prevLogTerm = std::stoi(field);
        // std::getline(ss, field, '\n'); args.entries.resize(std::stoi(field));
        // std::getline(ss, field, '\n'); args.leaderCommit = std::stoi(field);
        return args;
    }
};

} // namespace raft::codec
