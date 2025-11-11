#pragma once
#include "raft/types.h"
#include <string>
#include <sstream>
#include <optional>
#include <spdlog/spdlog.h>
namespace raft::codec {

class RaftCodec {
public:
    //---------struct to string---------
    //RequestVoteArgs to string
    static inline std::string encode(const type::RequestVoteArgs& args) {
        std::stringstream ss;
        ss << args.term << "\n" 
           << args.candidateId << "\n" 
           << args.lastLogIndex << "\n" 
           << args.lastLogTerm << "\n";
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
            ss << entry.index << " " << entry.term << " " << entry.command.size() << "\n";
            ss << entry.command << "\n";
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
    static inline std::string encode(const type::InstallSnapshotArgs& args) {
        std::stringstream ss;
        ss << args.term << "\n"
           << args.leaderId << "\n"
           << args.lastIncludedIndex << "\n"
           << args.lastIncludedTerm << "\n"
           << args.snapshot.size() << "\n"
           << args.snapshot <<"\n";
        return ss.str();
    }

    static inline std::string encode(const type::InstallSnapshotReply& reply) {
        return std::to_string(reply.term);
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
        ss << (votedFor ? std::to_string(*votedFor) : "-1") << "\n";
        ss << log.size() << "\n";
        for (const auto& entry : log) {
            ss << entry.index << " " << entry.term << " " << entry.command.size() << "\n";
            //'\n' in command is supported
            ss << entry.command << "\n";
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
        if (field.empty()) return {};
        reply.term = std::stoi(field);
        
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
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
            if(field.empty()) return {};
            std::stringstream header(field);
            type::LogEntry entry;
            std::size_t cmdLen = 0;
            // index and term
            if (!(header >> entry.index >> entry.term >> cmdLen)) return {}; 

            std::string commandPart;
            commandPart.resize(cmdLen);
            // rest of line is command
            ss.read(&commandPart[0],static_cast<std::streamsize>(cmdLen));
            if(ss.gcount() != static_cast<std::streamsize>(cmdLen)) return {};// not enough
            // if (!commandPart.empty() && commandPart[0] == ' ') commandPart.erase(0, 1);
            entry.command = std::move(commandPart);
            // consume the trailing newline after command if present
            if (ss.peek() == '\n') ss.get();
            args.entries[i] = std::move(entry);
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

    //string to InstallSnapshotArgs
    static inline type::InstallSnapshotArgs decodeInstallSnapshotArgs(const std::string& payload) {
        std::stringstream ss(data);
        type::InstallSnapshotArgs args{};
        std::string field;
        
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.term = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.leaderId = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.lastIncludedIndex = std::stoi(field);

        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        args.lastIncludedTerm = std::stoi(field);

        // Next line: snapshot size
        if (!std::getline(ss, field, '\n')) return {};
        if (field.empty()) return {};
        std::size_t snapSize = static_cast<std::size_t>(std::stoll(field));

        args.snapshot.resize(snapSize);
        if (snapSize > 0) {
            ss.read(&args.snapshot.data(), static_cast<std::streamsize>(snapSize));
            if (ss.gcount() != static_cast<std::streamsize>(snapSize)) return {};
            // consume the trailing newline after snapshot if present
            if (ss.peek() == '\n') ss.get();
        }

        return args;
    }

    //string to InstallSnapshotReply
    static inline type::InstallSnapshotReply decodeInstallSnapshotReply(const std::string& data) {
        std::stringstream ss(data);
        type::InstallSnapshotReply r;
        ss >> r.term;
        return r;
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
        logData.clear();    // ensure logData is empty before populating
        std::stringstream ss(data);
        std::string field;
        try{
            if (!std::getline(ss, field, '\n')) return false;
            if (field.empty())  return false;
            currentTerm = std::stoi(field);

            if (!std::getline(ss, field, '\n')) return false;
            if (field.empty()) return false;
            int v = std::stoi(field);
            if (v == -1) votedFor.reset();
            else votedFor = v;
            

            if (!std::getline(ss, field, '\n')) return false;
            if (field.empty()) return false;
            int logSize = std::stoi(field);
            if (logSize == 0) spdlog::debug("[Codec] Empty log state.");
            for (int i = 0; i < logSize; ++i) {
                if (!std::getline(ss, field)) return false; // not enough lines
                std::stringstream headerSS(field);
                type::LogEntry entry;
                size_t commandLen = 0;
                if (!(headerSS >> entry.index >> entry.term >> commandLen))
                    return false;
                
                //read precisely commandLen bytes
                std::string command(commandLen, '\0');
                ss.read(&command[0], commandLen);

                //skip '\n' at the end
                char newline;
                ss.get(newline);

                entry.command = std::move(command);
                logData.push_back(std::move(entry));
            }
            return true;
        }catch(...){
            spdlog::error("[Codec] Failed to decode Raft state due to malformed data.");
            return false;
        }
        return true;
    }
};// class RaftCodec

} // namespace raft::codec
