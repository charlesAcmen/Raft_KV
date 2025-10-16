#pragma once

#include "types.h"  // for type::LogEntry, type::Role, etc
#include <mutex>    //lock for mu_
#include <vector>
#include <unordered_map> //for nextIndex_ and matchIndex_
#include <optional> //optional for votedFor_ and RPC replies
#include <memory>   // for shared_ptr and unique_ptr
#include <atomic>   //running_ is atomic
#include <thread>   //for run() thread

namespace raft {
//---------- Forward declarations ----------
class IRaftTransport;
class ITimer;
class ITimerFactory;
// class IPersister;
class Raft {
    public:
        Raft(
            int me,
            const std::vector<int>& peers,
            std::shared_ptr<IRaftTransport> transport,
            std::shared_ptr<ITimerFactory> timerFactory = nullptr
            // std::shared_ptr<IPersister> persister = nullptr
        );

        ~Raft();
       
        // Start internal worker thread,transport,and timers.
        void Start();

        // Stop transport and timers.
        void Stop();

        // Block until internal thread exits (join).
        void Join();

    private:
        //----constants----
        // Heartbeat interval for leaders to send AppendEntries RPCs in milliseconds
        static constexpr std::chrono::milliseconds HEARTBEAT_INTERVAL{100};
        // Election timeout range in milliseconds (randomized per election)
        static constexpr std::chrono::milliseconds ELECTION_TIMEOUT_MIN{1000};
        static constexpr std::chrono::milliseconds ELECTION_TIMEOUT_MAX{2000};


        // Non-copyable
        Raft(const Raft&) = delete;
        Raft& operator=(const Raft&) = delete;

        // Internal thread main loop
        void run();

        // naming convention: Locked means caller must hold mu_ lock before calling


        //-------------------------------------
        //--------- Election control ----------
        //-------------------------------------
        void startElectionLocked();                   // Begin new election (called on timeout)


        //-------------------------------------
        //----------- RPC handlers ------------
        //-------------------------------------

        type::RequestVoteReply HandleRequestVote(const type::RequestVoteArgs& args);
        type::AppendEntriesReply HandleAppendEntries(const type::AppendEntriesArgs& args);


        //-------------------------------------
        //--------- Helper functions ----------
        //-------------------------------------
        // Send one RequestVote RPC
        std::optional<type::RequestVoteReply> sendRequestVoteRPC(int peerId);     
        // Send one AppendEntries RPC (heartbeat or log)
        std::optional<type::AppendEntriesReply> sendAppendEntriesRPC(int peerId);  
        // Send empty AppendEntries to all peers
        void broadcastHeartbeatLocked();            
        // Send heartbeat to one peer 
        std::optional<type::AppendEntriesReply> sendHeartbeatLocked(int peer);

        //-------------------------------------
        //---------- Role transtions ----------
        //-------------------------------------

        // Update term, clear vote, stop heartbeat timer,start election timer
        void becomeFollowerLocked(int32_t newTerm);  
        // Increment term, self-vote, reset election timer, start election 
        void becomeCandidateLocked();                 
        // stop election timer,start heartbeat timer,broadcast heartbeats
        void becomeLeaderLocked();                    


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
        // -------- Timer functions -----------
        //-------------------------------------

        //called when election timer fires
        //request lock and call becomeCandidateLocked()
        void onElectionTimeout();         
        //called when heartbeat timer fires  
        //request lock and broadcastHeartbeat and start heartbeat timer 
        //if is leader
        void onHeartbeatTimeout();          

        // Reset election timer with a new randomized timeout
        void resetElectionTimerLocked();
        // start heartbeat timer
        void resetHeartbeatTimerLocked();


        // internal data protected by mu_
        std::mutex mu_;

        // Raft identity and cluster
        const int me_;                      // this peer's id (index into peers_)
        const std::vector<int> peers_;      // peer ids (including me_)

        //------state------
        // all following codes are inline with Raft paper notations

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
        // (Reinitialized after election)
        std::unordered_map<int, int> nextIndex_;
        // for each server, index of the next log entry to send to that server
        //(initialized to leader last log index + 1)
        std::unordered_map<int, int> matchIndex_;
        // for each server, index of highest log entry known to be replicated on server
        //(initialized to 0, increases monotonically)



        // used to send RPCs to peers
        std::shared_ptr<IRaftTransport> transport_; 

        // timers,interface oriented,Runtime polymorphism
        //passed by constructor
        std::shared_ptr<ITimerFactory> timerFactory_;
        //created by timerFactory_
        std::unique_ptr<ITimer> electionTimer_;
        std::unique_ptr<ITimer> heartbeatTimer_;

        // background thread for running Raft
        std::thread thread_;        
        // flag to control thread loop
        std::atomic<bool> running_{false};
};

} // namespace raft
