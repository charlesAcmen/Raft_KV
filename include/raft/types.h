#pragma once


#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace raft {
namespace type{



// Role of a Raft peer
enum class Role{
    Follower,
    Candidate,
    Leader
};


// A minimal log entry structure. For 2A (election) entries are not required,
// but we keep a compact definition so types match later phases (2B/2C).
struct LogEntry {
    int32_t index{0}; // 1-based index
    int32_t term{0};  // term when entry was received by leader
    std::string command; // opaque command (may be empty for heartbeats)
};


// RequestVote RPC arguments (Raft paper §5.2).
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


// AppendEntries RPC args. For 2A we only need the heartbeat 
// (entries may be empty).
struct AppendEntriesArgs {
    int32_t term{0}; // leader’s term
    int32_t leaderId{0}; // so follower can redirect clients
    int32_t prevLogIndex{0}; // index of log entry immediately preceding new ones
    int32_t prevLogTerm{0}; // term of prevLogIndex entry
    std::vector<LogEntry> entries; // log entries to store 
    //(empty for heartbeat; may send more than one for efficiency)
    int32_t leaderCommit{0}; // leader’s commitIndex
};


// AppendEntries RPC reply.
struct AppendEntriesReply {
    int32_t term{0}; // currentTerm, for leader to update itself
    bool success{false}; // true if follower contained entry matching prevLogIndex/prevLogTerm
    
    int32_t conflictIndex{0};
    int32_t conflictTerm{0};
};

} // namespace type
} // namespace raft