#include "raft/raft.h"
#include "raft/timer_thread.h"
#include "raft/transport.h"
#include "raft/codec/raft_codec.h"
#include <spdlog/spdlog.h>
#include <random> 
namespace raft{
Raft::Raft(
    int me,
    const std::vector<int>& peers,
    std::shared_ptr<IRaftTransport> transport,
    // std::function<void(const type::LogEntry&)> applyCallback,
    // RaftConfig config,
    std::shared_ptr<ITimerFactory> timerFactory
    // std::shared_ptr<IPersister> persister
)
    :
    me_(me), 
    peers_(peers), 
    transport_(transport),
    timerFactory_(timerFactory)
{
    // -----------------------
    // Basic invariant checks
    // -----------------------
    // make sure the local id exists in the peer set (or at least warn).
    bool found_me = false;
    for (int p : peers_) {
        if (p == me_) { found_me = true; break; }
    }
    if (!found_me) {
        spdlog::warn("[Raft] constructor: my id not found in peers vector,continuing", me_);
    }

    // -----------------------
    // Register RPC handlers
    // -----------------------
    // register the two RPC handlers that accept a serialized payload and 
    // return a serialized reply. The transport is responsible for invoking
    // these lambdas when a remote node calls "AppendEntries" / "RequestVote".
    // The lambdas: decode -> call local handler function -> encode reply.
    transport_->RegisterRequestVoteHandler(
        [this](const std::string& payload) -> std::string {
            // decode incoming args, run local handler, encode reply
            try {
                type::RequestVoteArgs args = codec::RaftCodec::decodeRequestVoteArgs(payload);
                type::RequestVoteReply reply = this->HandleRequestVote(args);
                return codec::RaftCodec::encode(reply);
            } catch (const std::exception& e) {
                spdlog::error("[Raft] {} RequestVote handler exception: {}", this->me_, e.what());
                return std::string();// caller should check/interpret empty as failure
            }
        }
    );
    transport_->RegisterAppendEntriesHandler(
        [this](const std::string& payload) -> std::string {
            // decode incoming args, run local handler, encode reply
            try {
                type::AppendEntriesArgs args = codec::RaftCodec::decodeAppendEntriesArgs(payload);
                // HandleAppendEntries is expected to be a member that returns type::AppendEntriesReply
                type::AppendEntriesReply reply = this->HandleAppendEntries(args);
                return codec::RaftCodec::encode(reply);
            } catch (const std::exception& e) {
                // on decode/handler error, log and return empty/error payload
                spdlog::error("[Raft] {} AppendEntries handler exception: {}", this->me_, e.what());
                return std::string(); // caller should check/interpret empty as failure
            }
        }
    );

    // -----------------------
    // Initialize timers
    // -----------------------

    electionTimer_ = timerFactory_->CreateTimer([this]() {
        this->onElectionTimeout();
    });

    heartbeatTimer_ = timerFactory_->CreateTimer([this]() {
        this->onHeartbeatTimeout();
    });

    // keep constructor responsibility limited. Heavy initialization (timers,
    // persistent state restore, election timer start) should be done by an explicit Start()
    // or Init() method. Below we only do cheap, safe adjustments.

    // log constructed state for debugging.
    spdlog::info("[Raft] {} constructed with {} peers", me_, peers_.size());
}

Raft::~Raft() {
   transport_->Stop();
}

void Raft::Start() {
    // Start background components and threads.
    transport_->Start();
    // start election timer
    resetElectionTimerLocked();
    spdlog::info("[Raft] {} started", me_);
}  

//-------------------private methods-------------------
//--------- Election control ----------
void Raft::startElection(){

}

//---------- Role transtions ----------
void Raft::becomeFollower(int32_t newTerm){

}
void Raft::becomeCandidate(){

}
void Raft::becomeLeader(){

}


//----------- RPC handlers ------------
// Handle a RequestVote RPC from a candidate
type::RequestVoteReply Raft::HandleRequestVote(const type::RequestVoteArgs& args){
    std::lock_guard<std::mutex> lock(mu_);
    type::RequestVoteReply reply{};
    
    // Reply's term is always our current term initially
    reply.term = currentTerm_;

    // Step 1: If the term in the request is less than our term, reject
    if (args.term < currentTerm_) {
        reply.voteGranted = false;
        spdlog::info("[Raft] {} rejecting vote for {}: stale term ({} < {})",
            me_, args.candidateId, args.term, currentTerm_);
        return reply;
    }

    // Step 2: If the term in the request is greater than our term, update term and convert to follower
    if (args.term > currentTerm_) {
        currentTerm_ = args.term;
        votedFor_ = std::nullopt; // reset votedFor
        role_ = type::Role::Follower;
        spdlog::info("[Raft] {} updating term to {} and becoming follower", me_, currentTerm_);
    }

    // Step 3: vote for candidate only if candidate’s log is at least as up-to-date as receiver’s log
    bool logOk = (args.lastLogTerm > getLastLogTerm()) ||
                 (args.lastLogTerm == getLastLogTerm() && args.lastLogIndex >= getLastLogIndex());

    //have not voted this term or voted for candidate, and candidate's log is at least as up-to-date
    if ((votedFor_ == std::nullopt || votedFor_ == args.candidateId) && logOk) {
        votedFor_ = args.candidateId;
        reply.voteGranted = true;
        resetElectionTimerLocked(); // reset follower timer
        spdlog::info("[Raft] {} voted for {} in term {}", me_, args.candidateId, currentTerm_);
    } else {
        reply.voteGranted = false;
        spdlog::info("[Raft] {} rejecting vote for {} in term {}", me_, args.candidateId, currentTerm_);
    }

    return reply;
}
// Handle an AppendEntries RPC from the leader
type::AppendEntriesReply Raft::HandleAppendEntries(const type::AppendEntriesArgs& args){
    std::lock_guard<std::mutex> lock(mu_);
    type::AppendEntriesReply reply{};
    reply.term = currentTerm_;

    // Step 1: Reply false if term < currentTerm
    if (args.term < currentTerm_) {
        reply.success = false;
        spdlog::info("[Raft] {} rejecting vote for {}: stale term ({} < {})",
            me_, args.leaderId, args.term, currentTerm_);
        return reply;
    }

    // Step 2: If term > currentTerm, update and convert to follower
    if (args.term > currentTerm_) {
        currentTerm_ = args.term;
        role_ = type::Role::Follower;
        votedFor_ = std::nullopt; // reset votedFor
        spdlog::info("[Raft] {} updating term to {} and becoming follower", me_, currentTerm_);
    }

    // Step 3: Reset election timeout since valid leader contacted us
    // Receiving valid AppendEntries acts as heartbeat → reset timeout
    resetElectionTimerLocked();

    // Step 4: Check if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm
    // 如果日志中没有在 prevLogIndex 位置的条目，
    // 或者 有该条目但其 term 与 prevLogTerm 不匹配，
    // 那么就返回 false。
    if (args.prevLogIndex > getLastLogIndex() ||
        getLogTerm(args.prevLogIndex) != args.prevLogTerm) {
        reply.success = false;
        spdlog::info("[Raft] {} rejecting AppendEntries from {}: log inconsistency at prevLogIndex {}",
            me_, args.leaderId, args.prevLogIndex);
        return reply;
    }

    // Step 5: Append any new entries not already in the log
    // (Here we assume entries contain only sizes or actual entries in a real implementation)
    // If existing entries conflict with new ones (same index but different term), 
    // delete the existing entry and all that follow it
    for (size_t i = 0; i < args.entries.size(); ++i) {
        int index = args.prevLogIndex + 1 + i;
        if (index <= getLastLogIndex() && getLogTerm(index) != args.entries[i].term) {
            // Conflict, delete all entries from index onward
            deleteLogFromIndex(index);
        }
        if (index > getLastLogIndex()) {
            log_.push_back(args.entries[i]);
        }
    }

    // Step 6: Update commitIndex
    if (args.leaderCommit > commitIndex_) {
        commitIndex_ = std::min(args.leaderCommit, getLastLogIndex());
        applyLogs(); // apply committed logs to state machine
    }

    reply.success = true;
    return reply;
}

//---------- Log management -----------

int Raft::getLastLogIndex() const{
    std::lock_guard<std::mutex> lock(mu_);
    // If log is empty, return 0 as Raft paper defines first log index as 1
    return static_cast<int>(log_.size());
}
int Raft::getLastLogTerm() const{
    std::lock_guard<std::mutex> lock(mu_);
    // If log is empty, return 0 as initial term
    return log_.empty() ? 0 : log_.back().term;
}
int Raft::getLogTerm(int index) const{
    std::lock_guard<std::mutex> lock(mu_);
    // Log index starts at 1 in Raft, so adjust for 0-based vector
    if (index <= 0 || index > static_cast<int>(log_.size())) {
        // Out of range; return 0 as default term
        return 0;
    }
    return log_[index - 1].term;
}
/**
 * @brief Apply committed but not yet applied log entries to the state machine.
 * 
 * This function is typically called by the Raft main loop or background thread.
 * It checks the logs up to `commitIndex_` and applies all entries that have not
 * yet been applied (i.e., whose index > lastApplied_). 
 * 
 * After applying each log entry, `lastApplied_` is updated.
 */
void Raft::applyLogs(){
    std::lock_guard<std::mutex> lock(mu_);

    // Apply all committed entries not yet applied
    while (lastApplied_ < commitIndex_) {
        lastApplied_++;
        const auto &entry = log_.at(lastApplied_); // assuming log_ is 1-based index or adjusted
        // Here you should actually apply 'entry.command' to your state machine.
        // e.g. stateMachine.apply(entry.command);
        spdlog::info("[applyLogs] Applying log at index {} (term={})", lastApplied_, entry.term);
    }
}
/**
 * @brief Delete log entries starting from the given index (inclusive).
 * 
 * This function is used when a leader detects inconsistency in a follower's log.
 * For example, upon receiving an AppendEntries reply with `success = false`,
 * the leader will decrement `nextIndex` and retry; on the follower side,
 * it may delete conflicting entries to match the leader's log.
 *
 * @param index The starting index from which logs should be deleted (inclusive).
 */
void Raft::deleteLogFromIndex(int index){
    std::lock_guard<std::mutex> lock(mu_);
    
    if (index <= 0 || index > static_cast<int>(log_.size())) {
        spdlog::warn("[deleteLogFromIndex] Invalid index {}, log size={}", index, log_.size());
        return;
    }

    // Erase all entries from `index` to end
    log_.erase(log_.begin() + index, log_.end());
    spdlog::info("[deleteLogFromIndex] Deleted logs from index {} to end, remaining size={}", 
                 index, log_.size());
}

//--------- Helper functions ----------
void Raft::persistStateLocked(){

}          
void Raft::sendRequestVoteRPC(int peerId){

} 
void Raft::sendAppendEntriesRPC(int peerId){
    
}
void Raft::broadcastHeartbeat(){

}
void Raft::resetElectionTimerLocked(){
    // Reset election timer with a new randomized timeout
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(150, 300); // in milliseconds
    int timeout = dist(rng);
    electionTimer_->Reset(std::chrono::milliseconds(timeout));
    spdlog::info("[Raft] {} reset election timer to {} ms", me_, timeout);
}

// -------- Timer callbacks -----------
void Raft::onElectionTimeout(){

}
void Raft::onHeartbeatTimeout(){

}

//--------- Internal helpers ----------
}// namespace raft