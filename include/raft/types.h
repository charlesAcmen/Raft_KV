#pragma once
#include <cstdint>  // for int32_t,c standard types
#include <string>   
#include <vector>
#include <chrono>   // for time_point
// Basic types and structures for Raft consensus algorithm
// all following codes are inline with Raft paper notations
namespace raft::type{

// Role of a Raft peer
enum class Role{
    Follower,
    Candidate,
    Leader
};

// for now only support Unix domain socket transport
// In real world, we may want to support TCP/IP as well
struct PeerInfo {
    int id;                 // unique Raft node ID
    std::string sockPath;   // /tmp/raft-node-<id>.sock,Unix socket path for RPC
};


struct LogEntry {
    int32_t index{0}; // 1-based index in the raft log
    int32_t term{0};  // term when entry was received by leader
    std::string command; // opaque command (may be empty for heartbeats)
};

struct ApplyMsg{
    bool CommandValid{false}; // true if this is a newly committed log entry
    std::string Command;   // the command to apply to state machine
    int32_t CommandIndex{0}; // the index of the command in the log

    // For 2D:
    bool SnapshotValid{false}; // true if this is a snapshot to apply
    std::string Snapshot; // the snapshot to apply to state machine
    int32_t SnapshotTerm{0}; // the term of the last included log entry in the snapshot
    int32_t SnapshotIndex{0}; // the index of the last included log entry in the snapshot
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
    bool success{false}; // true if follower contained entry matching prevLogIndex and prevLogTerm
};
} // namespace  raft::type