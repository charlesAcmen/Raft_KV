#pragma once
#include "../types.h"
#include <string>
#include <sstream>
#include <optional>
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
        ss << args.term << "\n" 
           << args.leaderId << "\n" 
           << args.prevLogIndex << "\n" 
           << args.prevLogTerm << "\n" 
           << args.entries.size() << "\n" 
           << args.leaderCommit << "\n";
        
        // Serialize each log entry: index term command
        for (const auto& entry : args.entries) {
            ss << entry.index << " " << entry.term << " " << entry.command << "\n";
        }
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
    /**
     * @brief Serialize Raft's persistent fields (term, vote, log)
     *        into a byte stream for persisting.
     * 
     * @param currentTerm current term number
     * @param votedFor    ID of the candidate voted for
     * @param logData     serialized log entries (already encoded)
     * @return std::string encoded byte stream
     */
    static inline std::string encodeRaftState(
        int32_t currentTerm, std::optional<int32_t> votedFor, const std::vector<type::LogEntry>& log) {
        std::stringstream ss;
        ss << currentTerm << "\n";
        if(votedFor){
            ss << *votedFor << "\n";
        }else{
            ss << "null\n";
        }
        ss << log.size() << "\n";
        for (const auto& entry : log) {
            ss << entry.index << " " << entry.term << " " << entry.command << "\n";
        }
        return ss.str();
    }

    //---------string to struct---------







    //string to RequestVoteArgs
    static inline type::RequestVoteArgs decodeRequestVoteArgs(const std::string& payload) {
        struct type::RequestVoteArgs args{};
        std::stringstream ss(payload);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.term = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.candidateId = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.lastLogIndex = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.lastLogTerm = std::stoi(field);
        return args;
    }

    //string to RequestVoteReply
    static inline type::RequestVoteReply decodeRequestVoteReply(const std::string& payload){
        struct type::RequestVoteReply reply{};
        std::stringstream ss(payload);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if (!field.empty()) reply.term = std::stoi(field);
        else return {};
        if (!std::getline(ss, field, '\n')) return {};
        reply.voteGranted = (field == "1");
        return reply;
    }

    //string to AppendEntriesArgs
    static inline type::AppendEntriesArgs decodeAppendEntriesArgs(const std::string& payload) {
        struct type::AppendEntriesArgs args{};
        std::stringstream ss(payload);
        std::string field;
        
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.term = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.leaderId = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.prevLogIndex = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.prevLogTerm = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        int entryCount = std::stoi(field);
        args.entries.resize(entryCount);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.leaderCommit = std::stoi(field);

        // 7. parse each log entry: "index term command"
        for (int i = 0; i < entryCount; ++i) {
            if (!std::getline(ss, field)) return {}; // not enough lines
            std::stringstream entrySS(field);
            type::LogEntry entry;
            std::string commandPart;

            // index and term
            if (!(entrySS >> entry.index >> entry.term)) return {}; 

            // rest of line is command
            std::getline(entrySS, commandPart);
            if (!commandPart.empty() && commandPart[0] == ' ') commandPart.erase(0, 1);
            entry.command = commandPart;

            args.entries[i] = entry;
        }



        return args;
    }

    //string to AppendEntriesReply
    static inline type::AppendEntriesReply decodeAppendEntriesReply(const std::string& payload) {
        struct type::AppendEntriesReply reply{};
        std::stringstream ss(payload);
        std::string field;
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        reply.term = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        reply.success = (field == "1");
        return reply;
    }



    /**
     * @brief Deserialize Raft's persistent fields from a byte stream.
     * 
     * @param data serialized byte stream from persister
     * @param currentTerm output term
     * @param votedFor output votedFor
     * @param logData output log entries (serialized form)
     */
    static inline bool decodeRaftState(
        const std::string& data,
        int32_t& currentTerm, std::optional<int32_t>& votedFor, std::vector<type::LogEntry>& logData){
        if(!logData.empty()) return false;
        std::stringstream ss(data);
        std::string field;
        if (!std::getline(ss, field, '\n')) return false;
        if (field.empty()) return false;
        currentTerm = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return false;
        if (field.empty()) return false;
        if (field == "null") {
            votedFor.reset();
        } else {
            votedFor = std::stoi(field);
        }

        if (!std::getline(ss, field, '\n')) return false;
        if (field.empty()) return false;
        int logSize = std::stoi(field);

        for (int i = 0; i < logSize; ++i) {
            if (!std::getline(ss, field)) return false; // not enough lines
            std::stringstream entrySS(field);
            type::LogEntry entry;
            std::string commandPart;

            if(!(entrySS >> entry.index >> entry.term)) return false;
            std::getline(entrySS, commandPart);
            if (!commandPart.empty() && commandPart[0] == ' ') commandPart.erase(0, 1);
            entry.command = commandPart;
            logData.push_back(entry);
        }
        return true;
    }
};

} // namespace raft::codec
