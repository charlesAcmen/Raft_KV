#pragma once

#include "types.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <optional>
#include <functional>
#include <memory>
#include <atomic>

namespace raft {
    
class IRaftTransport;
class ITimer;
class ITimerFactory;
// RaftConfig holds tunable(可调) parameters affecting election timing. Tests
// should set these explicitly to reduce flakiness (or inject a virtual
// timer implementation via ITimerFactory).
// struct RaftConfig {
//     // Election timeout range in milliseconds: each follower chooses a random
//     // timeout uniformly in [electionTimeoutMinMs, electionTimeoutMaxMs].
//     int electionTimeoutMinMs{150};
//     int electionTimeoutMaxMs{300};

//     // How long a leader waits between heartbeats (AppendEntries with empty
//     // entries). This is usually much shorter than election timeout.
//     int heartbeatIntervalMs{50};

//     // Timeout to wait for an RPC reply before considering it failed.
//     int rpcTimeoutMs{100};
// };


// Raft implements the core Raft peer state.
// This header intentionally restricts the public API to
// the minimal set required for election behaviour and for integrating
class Raft {
    public:
        // me: this peer's id (index into peers_).
        // transport: implementation that sends RequestVote/AppendEntries RPCs.
        Raft(
            int me,
            const std::vector<int>& peers,
            std::shared_ptr<IRaftTransport> transport,
            // std::function<void(const type::LogEntry&)> applyCallback,
            // RaftConfig config = RaftConfig(),
            std::shared_ptr<ITimerFactory> timerFactory = nullptr
            // std::shared_ptr<IPersister> persister = nullptr
        );

        ~Raft();
       
        void Start();

    private:
        // Non-copyable
        Raft(const Raft&) = delete;
        Raft& operator=(const Raft&) = delete;

        //-------------------------------------
        //--------- Election control ----------
        //-------------------------------------
        void startElection();                   // Begin new election (called on timeout)

        //-------------------------------------
        //---------- Role transtions ----------
        //-------------------------------------

        void becomeFollower(int32_t newTerm);   // Step down to follower
        void becomeCandidate();                 // Increment term, self-vote, send RequestVote RPCs
        void becomeLeader();                    // Initialize nextIndex, matchIndex, send heartbeats

        //-------------------------------------
        //----------- RPC handlers ------------
        //-------------------------------------

        type::RequestVoteReply HandleRequestVote(const type::RequestVoteArgs& args);
        type::AppendEntriesReply HandleAppendEntries(const type::AppendEntriesArgs& args);

        //-------------------------------------
        //---------- Log management -----------
        //-------------------------------------

        // Returns the index of the last log entry (0 if no entries)
        int getLastLogIndex() const;
        // Returns the term of the last log entry (0 if no entries)
        int getLastLogTerm() const;
        // Returns the term of log at given index (1-based)
        int getLogTerm(int index) const;
        // Applies committed log entries to the state machine
        void applyLogs();
        void deleteLogFromIndex(int index);

        //-------------------------------------
        //--------- Helper functions ----------
        //-------------------------------------

        void persistStateLocked();              // Save currentTerm, votedFor, log[] to disk
        void sendRequestVoteRPC(int peerId);    // Send one RequestVote RPC to a peer
        void sendAppendEntriesRPC(int peerId);  // Send one AppendEntries RPC (heartbeat or log)
        void broadcastHeartbeat();              // Send empty AppendEntries to all peers
        void resetElectionTimerLocked();
        void resetHeartbeatTimerLocked();

        //-------------------------------------
        // -------- Timer callbacks -----------
        //-------------------------------------

        void onElectionTimeout();           
        void onHeartbeatTimeout();              

        //-------------------------------------
        //--------- Internal helpers ----------
        //-------------------------------------

        // Internal data protected by mu_. Any access to these fields must hold
        // mu_ to ensure correctness unless otherwise noted in comments.
        mutable std::mutex mu_;

        // Raft identity and cluster
        const int me_;                      // this peer's id (index into peers_)
        const std::vector<int> peers_;      // peer ids (including me_)

        //------state------
        // Persistent state on all servers
        type::Role role_{type::Role::Follower};
        int32_t currentTerm_{0};            // latest term server has seen(initialized to 0 on first boot, 
        //increases monotonically)
        std::optional<int32_t> votedFor_;   // candidateId that received vote in currentTerm
        // or nullopt if none
        std::vector<type::LogEntry> log_;         // log entries; each entry contains command for state machine,
        // and term when entry was received by leader(first index is 1)

        // Volatile state on all servers
        int32_t commitIndex_{0};        // index of highest log entry known to be committed
        //(initialized to 0, increases monotonically)
        int32_t lastApplied_{0};        // index of highest log entry applied to state machine
        //(initialized to 0, increases monotonically)

        // Volatile state on leaders (reinitialized after election)
        // std::unordered_map<int, int> nextIndex_;
        // std::unordered_map<int, int> matchIndex_;



        // used to send RPCs to peers
        std::shared_ptr<IRaftTransport> transport_; 

        // timers
        std::shared_ptr<ITimerFactory> timerFactory_;
        std::unique_ptr<ITimer> electionTimer_;
        std::unique_ptr<ITimer> heartbeatTimer_;
};

} // namespace raft
