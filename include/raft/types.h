#pragma once


#include <cstdint>  // for int32_t
#include <string>
#include <vector>
#include <spdlog/spdlog.h>


namespace raft::type{



// Role of a Raft peer
enum class Role{
    Follower,
    Candidate,
    Leader
};

struct PeerInfo {
    int id;                 // unique Raft node ID
    std::string sockPath;   // /tmp/raft-node-<id>.sock,Unix socket path for RPC
};


// A minimal log entry structure. For 2A (election) entries are not required,
// but we keep a compact definition so types match later phases (2B/2C).
struct LogEntry {
    int32_t index{0}; // 1-based index
    int32_t term{0};  // term when entry was received by leader
    std::string command; // opaque command (may be empty for heartbeats)
};


// RequestVote RPC arguments
struct RequestVoteArgs {
    int32_t term{0}; // candidate’s term
    int32_t candidateId{0}; // candidate requesting vote
    int32_t lastLogIndex{0}; // index of candidate’s last log entry
    int32_t lastLogTerm{0}; // term of candidate’s last log entry
};


// RequestVote RPC reply
struct RequestVoteReply {
    int32_t term{0}; // currentTerm, for candidate to update itself
    bool voteGranted{false};   //true means candidate received vote
};


// AppendEntries RPC args
struct AppendEntriesArgs {
    int32_t term{0}; // leader’s term
    int32_t leaderId{0}; // so follower can redirect clients
    int32_t prevLogIndex{0}; // index of log entry immediately preceding new ones
    int32_t prevLogTerm{0}; // term of prevLogIndex entry
    std::vector<LogEntry> entries; // log entries to store 
    //(empty for heartbeat; may send more than one for efficiency)
    int32_t leaderCommit{0}; // leader’s commitIndex
};


// AppendEntries RPC reply
struct AppendEntriesReply {
    int32_t term{0}; // currentTerm, for leader to update itself
    bool success{false}; // true if follower contained entry matching prevLogIndex/prevLogTerm
};

inline void PrintRequestVoteArgs(const RequestVoteArgs& args) {
    spdlog::info("[RequestVoteArgs] term={}, candidateId={}, lastLogIndex={}, lastLogTerm={}",
                 args.term, args.candidateId, args.lastLogIndex, args.lastLogTerm);
}

inline void PrintRequestVoteReply(const RequestVoteReply& reply) {
    spdlog::info("[RequestVoteReply] term={}, voteGranted={}",
                 reply.term, reply.voteGranted ? "true" : "false");
}


} // namespace  raft::type