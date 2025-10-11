#pragma once

#include "types.h"
#include "transport.h"
#include "timer.h"
#include "persister.h"

#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

namespace raft {

// Forward-declarations of RPC integration points.existing RPC
// abstraction (client/server/IMessageCodec) can be adapted to call
// Raft::HandleRequestVote and Raft::HandleAppendEntries when RPCs arrive.
// Example: rpcServer.registerHandler("RequestVote", [raftPtr](...) {
//              return raftPtr->HandleRequestVote(...);
//          });
namespace rpc {
    class RpcServer;    // RPC server abstraction
    class RpcClient;    // RPC client abstraction
    class IMessageCodec; // message codec abstraction
}

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
        // applyCallback: invoked for each committed log entry (not used by 2A).
        // transport: implementation that sends RequestVote/AppendEntries RPCs.
        // timerFactory: optional factory; if nullptr a default real-time factory
        //               must be provided in the .cpp implementation.
        // persister: optional persistence; for 2A it may be nullptr.
        Raft(int me,
            const std::vector<int>& peers,
            std::shared_ptr<IRaftTransport> transport,
            std::function<void(const LogEntry&)> applyCallback,
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
        void HandleRequestVote(const RequestVoteArgs& args, RequestVoteReply& reply);
        void HandleAppendEntries(const AppendEntriesArgs& args, AppendEntriesReply& reply);

        // The following getters are primarily for testing and debug visibility.
        int GetId() const { return me_; }
        Role GetRole();
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

        // Volatile state on all servers (Raft paper §3.1)
        Role role_{Role::Follower};
        int32_t currentTerm_{0};            // latest term server has seen
        std::optional<int32_t> votedFor_;   // candidateId that received vote in currentTerm

        // Persistent log (kept for compatibility; may be empty for 2A)
        std::vector<LogEntry> log_;

        // Volatile state on all servers
        int32_t commitIndex_{0};
        int32_t lastApplied_{0};

        // Transport and integration hooks
        std::shared_ptr<IRaftTransport> transport_; // used to send RPCs to peers
        std::function<void(const LogEntry&)> applyCallback_; // for committed entries

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
