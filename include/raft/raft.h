#pragma once

#include "types.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <optional>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace raft {
    
class IRaftTransport;
class ITimer;
class ITimerFactory;
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
       
        // Start internal worker thread and transport/timers.
        void Start();

        // Stop background work.
        // After Stop(), the worker thread should exit soon.
        void Stop();

        // Block until internal thread exits (join). Safe to call multiple times.
        void Join();

    private:
        // Non-copyable
        Raft(const Raft&) = delete;
        Raft& operator=(const Raft&) = delete;

        void run();

        //-------------------------------------
        //--------- Election control ----------
        //-------------------------------------
        void startElectionLocked();                   // Begin new election (called on timeout)

        //-------------------------------------
        //---------- Role transtions ----------
        //-------------------------------------

        void becomeFollowerLocked(int32_t newTerm);   // Step down to follower
        void becomeCandidateLocked();                 // Increment term, self-vote, send RequestVote RPCs
        void becomeLeaderLocked();                    // Initialize nextIndex, matchIndex, send heartbeats

        //-------------------------------------
        //----------- RPC handlers ------------
        //-------------------------------------

        type::RequestVoteReply HandleRequestVote(const type::RequestVoteArgs& args);
        type::AppendEntriesReply HandleAppendEntries(const type::AppendEntriesArgs& args);

        //-------------------------------------
        //---------- Log management -----------
        //-------------------------------------

        // Returns the index of the last log entry (0 if no entries)
        int getLastLogIndexLocked() const;
        // Returns the term of the last log entry (0 if no entries)
        int getLastLogTermLocked() const;
        // Returns the term of log at given index (1-based)
        int getLogTermLocked(int index) const;
        int getPrevLogIndexForLocked(int peerId) const;
        int getPrevLogTermForLocked(int peerId) const;
        std::vector<type::LogEntry> getEntriesToSendLocked(int peerId) const;
        // Applies committed log entries to the state machine
        void applyLogsLocked();
        void deleteLogFromIndexLocked(int index);

        //-------------------------------------
        //--------- Helper functions ----------
        //-------------------------------------
        std::optional<type::RequestVoteReply> sendRequestVoteRPC(int peerId);     // Send one RequestVote RPC
        std::optional<type::AppendEntriesReply> sendAppendEntriesRPC(int peerId);  // Send one AppendEntries RPC (heartbeat or log)
        
        void broadcastHeartbeatLocked();              // Send empty AppendEntries to all peers
        std::optional<type::AppendEntriesReply> sendHeartbeatLocked(int peer);
        void resetElectionTimerLocked();
        void resetHeartbeatTimerLocked();

        //-------------------------------------
        // -------- Timer callbacks -----------
        //-------------------------------------

        void onElectionTimeout();           
        void onHeartbeatTimeout();              
        // int calledCount_{0};




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
        std::unordered_map<int, int> nextIndex_;
        std::unordered_map<int, int> matchIndex_;



        // used to send RPCs to peers
        std::shared_ptr<IRaftTransport> transport_; 

        // timers
        std::shared_ptr<ITimerFactory> timerFactory_;
        std::unique_ptr<ITimer> electionTimer_;
        std::unique_ptr<ITimer> heartbeatTimer_;


        std::thread thread_;        // internal worker thread
        std::atomic<bool> running_{false};
};

} // namespace raft
