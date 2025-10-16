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
):me_(me), peers_(peers), transport_(transport),timerFactory_(timerFactory){

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
}

Raft::~Raft() {
    Stop();
    Join();
}

void Raft::Start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        spdlog::warn("[Raft] {} Start() called but Raft is already running", me_);
        // already started
        return;
    }
    // Start background components and threads.
    transport_->Start();
    // start election timer
    resetElectionTimerLocked();
    // do not start heartbeat timer yet cuz only leader uses it
    spdlog::info("[Raft] {} started", me_);
    thread_ = std::thread([this]() { this->run(); });
}  

void Raft::Stop(){
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        // already stopped or never started
        return;
    }
    transport_->Stop(); 
    electionTimer_->Stop(); 
    heartbeatTimer_->Stop();
    spdlog::info("[Raft] {} Stopped", me_);
}

void Raft::Join() {
    if (thread_.joinable()) {
        // spdlog::info("[Raft] {} Join() - waiting thread to exit", me_);
        thread_.join();
        // spdlog::info("[Raft] {} Join() - thread exited", me_);
    }
}

//-------------------private methods-------------------
void Raft::run() {
    while (running_) {
        // ==========================
        // Sleep briefly to reduce busy wait
        // ==========================
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // spdlog::info("[Raft] Node {} run loop exited", me_);
}




//--------- Election control ----------
void Raft::startElectionLocked(){
    spdlog::info("[Raft] Node {} starting election for term {}", me_, currentTerm_);

    int votesGranted = 1; // vote for self
    int majority = (peers_.size() / 2) + 1;

    // Send RequestVote RPCs to all peers
    for (const auto& peer : peers_) {
        if (peer == me_) continue; // skip self
        std::optional<type::RequestVoteReply> reply = sendRequestVoteRPC(peer);
        if(reply){
            if(reply->voteGranted){
                // handle vote granted
                votesGranted++;
                spdlog::info("[Raft] Node {} received vote from peer {} (total votes={})", me_, peer, votesGranted);
                // check if won majority
                if (votesGranted >= majority && role_ == type::Role::Candidate) {
                    becomeLeaderLocked(); // convert to Leader
                    break; // no need to send more votes
                }
            }
            else{
                // handle vote denied
                spdlog::info("[Raft] Node {} vote denied by peer {} (peer term={})", me_, peer, reply->term);
                // if peer's term > currentTerm_, step down
                if (reply->term > currentTerm_) {
                    becomeFollowerLocked(reply->term);
                    break;
                }
            }
        }else{
            spdlog::warn("[Raft] Node {} failed to get reply from peer {}", me_, peer);
            //TODO: handle no reply (network failure, timeout)
        }
    }
}

//---------- Role transtions ----------
// keep idempotent
void Raft::becomeFollowerLocked(int32_t newTerm){
    if (role_ == type::Role::Follower && currentTerm_ == newTerm) {
        // already follower in this term
        return;
    }
    currentTerm_ = newTerm;
    role_ = type::Role::Follower;
    spdlog::info("[Raft] {} becomes Follower (term={})", me_, currentTerm_);
    votedFor_ = std::nullopt;
    // stop heartbeat timer if running
    heartbeatTimer_->Stop();
    // reset election timer
    resetElectionTimerLocked();
}
void Raft::becomeCandidateLocked(){
    if (role_ == type::Role::Candidate) {
        return; // already candidate
    }
    // Increment current term and convert to candidate
    currentTerm_++;
    role_ = type::Role::Candidate;
    spdlog::info("[Raft] {} becomes candidate (term={})", me_, currentTerm_);
    votedFor_ = me_;
    resetElectionTimerLocked();
    startElectionLocked();
}
void Raft::becomeLeaderLocked(){
    if (role_ == type::Role::Leader) {
        return; // already leader
    }
    role_ = type::Role::Leader;
    spdlog::info("[Raft] {} becomes Leader (term={})", me_, currentTerm_);
    electionTimer_->Stop();
    resetHeartbeatTimerLocked();
    broadcastHeartbeatLocked();   
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
        becomeFollowerLocked(args.term);
        // spdlog::info("[Raft] {} updating term to {}", me_, currentTerm_);
    }

    // Step 3: vote for candidate only if candidate’s log is at least as up-to-date as receiver’s log
    bool logOk = (args.lastLogTerm > getLastLogTermLocked()) ||
                 (args.lastLogTerm == getLastLogTermLocked() && args.lastLogIndex >= getLastLogIndexLocked());

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
        becomeFollowerLocked(args.term); 
        spdlog::info("[Raft] {} updating term to {} and becomes follower", me_, currentTerm_);
    }else{
        // Step 3: Reset election timeout since valid leader contacted us
        // Receiving valid AppendEntries acts as heartbeat → reset timeout
        resetElectionTimerLocked();
    }

    // Step 4: Check if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm
    // 如果日志中没有在 prevLogIndex 位置的条目，
    // 或者 有该条目但其 term 与 prevLogTerm 不匹配，
    // 那么就返回 false。
    if (args.prevLogIndex > getLastLogIndexLocked() ||
        getLogTermLocked(args.prevLogIndex) != args.prevLogTerm) {
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
        if (index <= getLastLogIndexLocked() && getLogTermLocked(index) != args.entries[i].term) {
            // Conflict, delete all entries from index onward
            deleteLogFromIndexLocked(index);
        }
        if (index > getLastLogIndexLocked()) {
            log_.push_back(args.entries[i]);
        }
    }

    // Step 6: Update commitIndex
    if (args.leaderCommit > commitIndex_) {
        commitIndex_ = std::min(args.leaderCommit, getLastLogIndexLocked());
        applyLogsLocked(); // apply committed logs to state machine
    }

    reply.success = true;
    return reply;
}

//---------- Log management -----------

int Raft::getLastLogIndexLocked() const{
    // If log is empty, return 0 , first log index as 1
    return static_cast<int>(log_.size());
}
int Raft::getLastLogTermLocked() const{
    // If log is empty, return 0 as initial term
    return log_.empty() ? 0 : log_.back().term;
}
int Raft::getLogTermLocked(int index) const{
    // Log index starts at 1 in Raft, so adjust for 0-based vector
    if (index <= 0 || index > static_cast<int>(log_.size())) {
        // Out of range; return 0 as default term
        return 0;
    }
    return log_[index - 1].term;
}
// Returns the log index immediately before what should be sent to the given peer.
int Raft::getPrevLogIndexForLocked(int peerId) const {
    // nextIndex_[peerId] 指向要发送的下一条日志，因此前一条就是 prevLogIndex。
    return nextIndex_.at(peerId) - 1;
}

// Returns the term of the log entry immediately before what should be sent to the given peer.
int Raft::getPrevLogTermForLocked(int peerId) const {
    int prevIndex = getPrevLogIndexForLocked(peerId);
    return prevIndex > 0 ? getLogTermLocked(prevIndex) : 0;
}

// Returns the log entries to send to the given peer for AppendEntries RPC.
// Usually from nextIndex[peerId] to the end of the log.
std::vector<type::LogEntry> Raft::getEntriesToSendLocked(int peerId) const {
    int start = nextIndex_.at(peerId);
    if (start > getLastLogIndexLocked()) return {};
    return std::vector<type::LogEntry>(log_.begin() + start, log_.end());
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
void Raft::applyLogsLocked(){
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
void Raft::deleteLogFromIndexLocked(int index){
    
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
// RVO ensures that returning the struct avoids any unnecessary copies.
std::optional<type::RequestVoteReply> Raft::sendRequestVoteRPC(int peerId){
    // spdlog::info("[Raft] {} sending RequestVote RPC to peer {}", me_, peerId);
    type::RequestVoteArgs args{};
    args.term = currentTerm_;
    args.candidateId = me_;
    args.lastLogIndex = getLastLogIndexLocked();
    args.lastLogTerm = getLastLogTermLocked();
    type::RequestVoteReply reply{};
    bool success = transport_->RequestVoteRPC(
        peerId, args,reply,std::chrono::milliseconds(100));
    if (!success) {
        spdlog::warn("[Raft] Failed to send RequestVote RPC to peer {}", peerId);
        return std::nullopt;
    }
    return reply;
} 
std::optional<type::AppendEntriesReply> Raft::sendAppendEntriesRPC(int peerId){
    type::AppendEntriesArgs args{};
    args.term = currentTerm_;
    args.leaderId = me_;
    args.prevLogIndex = getLastLogIndexLocked();
    args.prevLogTerm = getLastLogTermLocked();
    args.entries = {};
    args.leaderCommit = commitIndex_;

    type::AppendEntriesReply reply{};
    bool success = transport_->AppendEntriesRPC(
        peerId, args, reply, std::chrono::milliseconds(100)
    );

    if (!success) {
        spdlog::warn("[Raft] Failed to send AppendEntries RPC to peer {}", peerId);
        return std::nullopt;
    }

    return reply;
}
void Raft::broadcastHeartbeatLocked(){
    for(const auto& peer : peers_){
        if(peer == me_) continue; // skip self
        std::optional<type::AppendEntriesReply> reply = 
                    sendHeartbeatLocked(peer);
        if (reply) {
            if (reply->term > currentTerm_) {
                becomeFollowerLocked(reply->term);
                spdlog::info("[Raft] Node {} stepping down to Follower due to higher term from {}", me_, peer);
                break;
            }
            else {
                spdlog::info("[Raft] Node {} heartbeat acknowledged by {}", me_, peer);
            }
        } else {
            spdlog::warn("[Raft] Node {} heartbeat to {} failed", me_, peer);
        }
    }
}
void Raft::resetElectionTimerLocked(){
    // Reset election timer with a new randomized timeout
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1500, 3000); // in milliseconds
    int timeout = dist(rng);
    electionTimer_->Reset(std::chrono::milliseconds(timeout));
}
void Raft::resetHeartbeatTimerLocked(){
    // Reset heartbeat timer to a fixed interval (e.g., 50ms)
    int heartbeatInterval = 100; // in milliseconds
    heartbeatTimer_->Reset(std::chrono::milliseconds(heartbeatInterval));
}

// -------- Timer callbacks -----------
// Called when the election timer times out.
// This function triggers the start of a new election by incrementing the term
// and sending RequestVote RPCs to all other peers.
void Raft::onElectionTimeout(){
    std::lock_guard<std::mutex> lock(mu_);
    spdlog::info("[Raft] Election timeout occurred on node {}.", me_);
    becomeCandidateLocked();
}
// Called when the heartbeat timer times out.
// This function triggers sending AppendEntries (heartbeat) RPCs to all followers
// if the node is the leader.
void Raft::onHeartbeatTimeout(){
    std::lock_guard<std::mutex> lock(mu_);
    // calledCount_++;
    // spdlog::info("[Raft] Heartbeat timeout occurred on node {} (count={}).", me_, calledCount_);
    if (role_ != type::Role::Leader) {
        spdlog::error("[Raft] Heartbeat timeout, but node {} is not the leader.", me_);
        return;
    }
    else{
        broadcastHeartbeatLocked();
        resetHeartbeatTimerLocked();
    }
}
std::optional<type::AppendEntriesReply> Raft::sendHeartbeatLocked(int peer){
    // spdlog::info("[Raft] {} Sending heartbeat AppendEntries to peer {}.", me_,peer);
    type::AppendEntriesArgs args{};
    args.term = currentTerm_;
    args.leaderId = me_;
    args.prevLogIndex = getLastLogIndexLocked();
    args.prevLogTerm = getLastLogTermLocked();
    args.entries = {}; // empty for heartbeat
    args.leaderCommit = commitIndex_;

    
    type::AppendEntriesReply reply{};
    transport_->AppendEntriesRPC(peer, args, reply, std::chrono::milliseconds(100));
    return reply;
}

}// namespace raft