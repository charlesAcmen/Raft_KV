#pragma once

#include "types.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

namespace raft {
    
class IRaftTransport;
class ITimer;
class ITimerFactory;
class IPersister;


// RaftConfig holds tunable(可调) parameters affecting election timing. Tests
// should set these explicitly to reduce flakiness (or inject a virtual
// timer implementation via ITimerFactory).
struct RaftConfig {
    // Election timeout range in milliseconds: each follower chooses a random
    // timeout uniformly in [electionTimeoutMinMs, electionTimeoutMaxMs].
    int electionTimeoutMinMs{150};
    int electionTimeoutMaxMs{300};

    // How long a leader waits between heartbeats (AppendEntries with empty
    // entries). This is usually much shorter than election timeout.
    int heartbeatIntervalMs{50};

    // Timeout to wait for an RPC reply before considering it failed.
    int rpcTimeoutMs{100};
};


// Raft implements the core Raft peer state.
// This header intentionally restricts the public API to
// the minimal set required for election behaviour and for integrating
class Raft {
    public:
        // me: this peer's id (index into peers_).
        // applyCallback: invoked for each committed log entry
        // transport: implementation that sends RequestVote/AppendEntries RPCs.
        // timerFactory: optional factory; if nullptr a default real-time factory
        //               must be provided in the .cpp implementation.
        // persister: optional persistence;
        Raft(int me,
            const std::vector<int>& peers,
            std::shared_ptr<IRaftTransport> transport,
            std::function<void(const type::LogEntry&)> applyCallback,
            RaftConfig config = RaftConfig(),
            std::shared_ptr<ITimerFactory> timerFactory = nullptr,
            std::shared_ptr<IPersister> persister = nullptr);

        ~Raft();

        // Start is the client-facing API to propose a new command. For 2A it
        // can return (index, term, isLeader=false) since log replication is not
        // implemented yet. Included for API compatibility with later labs.
        // index: the log index the command would occupy (if leader).
        // term: currentTerm when Start was called.
        // isLeader: whether this node believes it is the leader.
        std::tuple<int,int,bool> Start(const std::string& command);

        // GetState returns the current term and whether this node believes it
        // is the leader. Thread-safe.
        std::pair<int,bool> GetState();

        // Kill stops the Raft peer and all internal background activity. After
        // Kill returns, the object may be destroyed. Kill should not delete
        // persisted state (that is the test harness' responsibility).
        void Kill();

        // RPC handlers: these methods implement the server-side semantics of the
        // RequestVote and AppendEntries RPCs. Your RPC server should forward
        // incoming requests to these methods. They are thread-safe and may be
        // invoked concurrently.
        void HandleRequestVote(const type::RequestVoteArgs& args, type::RequestVoteReply& reply);
        void HandleAppendEntries(const type::AppendEntriesArgs& args, type::AppendEntriesReply& reply);

        // The following getters are primarily for testing and debug visibility.
        int GetId() const { return me_; }
        type::Role GetRole();
        int GetCurrentTerm();
        std::optional<int> GetVotedFor();

    private:
        // Non-copyable
        Raft(const Raft&) = delete;
        Raft& operator=(const Raft&) = delete;

        // Internal helper to start a new election (become Candidate).
        // It spawns background RPCs to peers; implementation must ensure that
        // mu_ lock is not held while performing remote calls.
        void startElection();

        // Transition helpers (must be called under lock mu_ unless otherwise
        // documented). They encapsulate paper semantics for state transition.
        void becomeFollower(int32_t newTerm);
        void becomeCandidate();
        void becomeLeader();

        // Reset (or start) the election timer with a randomized deadline.
        void resetElectionTimerLocked(); // expects mu_ held

        // Callback invoked when the election timer fires.
        void electionTimeoutHandler();

        // Internal data protected by mu_. Any access to these fields must hold
        // mu_ to ensure correctness unless otherwise noted in comments.
        std::mutex mu_;

        // Raft identity and cluster
        const int me_;                      // this peer's id (index into peers_)
        const std::vector<int> peers_;      // peer ids (including me_)

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



        // Transport and integration hooks
        std::shared_ptr<IRaftTransport> transport_; // used to send RPCs to peers
        std::function<void(const type::LogEntry&)> applyCallback_; // for committed entries

        // Timers
        RaftConfig config_;
        std::shared_ptr<ITimerFactory> timerFactory_;
        std::unique_ptr<ITimer> electionTimer_; // guards election timeout

        // Background control
        std::atomic<bool> running_{true};

        // Persistence
        std::shared_ptr<IPersister> persister_;

        // Condition variable used to coordinate election results (e.g. waiting
        // for majority votes). Implementations may use finer-grained sync.
        std::condition_variable cv_; // used together with mu_
};

} // namespace raft
