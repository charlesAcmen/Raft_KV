#pragma once
#include "types.h"
#include <string>
#include <sstream>

namespace raft::codec {

class RaftCodec {
public:
    static inline std::string encode(const type::RequestVoteArgs& args) {
        std::stringstream ss;
        ss << args.term << "," << args.candidateId
           << "," << args.lastLogIndex << "," << args.lastLogTerm;
        return ss.str();
    }

    static inline type::RequestVoteArgs decodeRequestVote(const std::string& payload) {
        type::RequestVoteArgs args;
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, ','); args.term = std::stoi(field);
        std::getline(ss, field, ','); args.candidateId = std::stoi(field);
        std::getline(ss, field, ','); args.lastLogIndex = std::stoi(field);
        std::getline(ss, field, ','); args.lastLogTerm = std::stoi(field);
        return args;
    }

    static inline std::string encode(const type::AppendEntriesArgs& args) {
        std::stringstream ss;
        ss << args.term << "," << args.leaderId << "," << args.prevLogIndex
           << "," << args.prevLogTerm << "," << args.entries.size()
           << "," << args.leaderCommit;
        return ss.str();
    }

    static inline type::AppendEntriesReply decodeAppendEntries(const std::string& payload) {
        type::AppendEntriesReply args;
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, ','); args.term = std::stoi(field);
        std::getline(ss, field, ','); args.leaderId = std::stoi(field);
        std::getline(ss, field, ','); args.prevLogIndex = std::stoi(field);
        std::getline(ss, field, ','); args.prevLogTerm = std::stoi(field);
        std::getline(ss, field, ','); args.entries.resize(std::stoi(field));
        std::getline(ss, field, ','); args.leaderCommit = std::stoi(field);
        return args;
    }
};

} // namespace raft::codec
