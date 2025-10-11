#pragma once
#include "../types.h"
#include <string>
#include <sstream>

namespace raft::codec {

class RaftCodec {
public:
    //---------struct to string---------
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
        /*
        only pass entries size here for now for simplicity
        when dealing with log replication, we need to serialize each entry
        */
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



    //---------string to struct---------







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

    //string to RequestVoteReply
    static inline type::RequestVoteReply decodeRequestVoteReply(const std::string& payload){
        struct type::RequestVoteReply reply{};
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, '\n'); reply.term = std::stoi(field);
        //"1" means true
        std::getline(ss, field, '\n'); reply.voteGranted = (field == "1");
        return reply;
    }

    //string to AppendEntriesArgs
    static inline type::AppendEntriesArgs decodeAppendEntriesArgs(const std::string& payload) {
        struct type::AppendEntriesArgs args{};
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, '\n'); args.term = std::stoi(field);
        std::getline(ss, field, '\n'); args.leaderId = std::stoi(field);
        std::getline(ss, field, '\n'); args.prevLogIndex = std::stoi(field);
        std::getline(ss, field, '\n'); args.prevLogTerm = std::stoi(field);
        std::getline(ss, field, '\n'); args.entries.resize(std::stoi(field));
        std::getline(ss, field, '\n'); args.leaderCommit = std::stoi(field);
        return args;
    }

    //string to AppendEntriesReply
    static inline type::AppendEntriesReply decodeAppendEntriesReply(const std::string& payload) {
        struct type::AppendEntriesReply reply{};
        std::stringstream ss(payload);
        std::string field;
        std::getline(ss, field, '\n'); reply.term = std::stoi(field);
        //"1" means true
        std::getline(ss, field, '\n'); reply.success = (field == "1");
        return reply;
    }
};

} // namespace raft::codec
