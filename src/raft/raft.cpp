#include "raft/raft.h"
#include "raft/timer_thread.h"      // complete definition of thread timerFactory and thread Timer
#include "raft/transport.h"         // complete definition of IRaftTransport
#include "raft/codec/raft_codec.h"  // used in register rpc handlers
#include <spdlog/spdlog.h>
#include <random>                   // for random election timeout 
namespace raft{
//-------------------public methods-------------------
Raft::Raft(int me,const std::vector<int>& peers,
    std::shared_ptr<IRaftTransport> transport)
    :me_(me), peers_(peers), transport_(transport){
    spdlog::set_pattern("[%l] %v");
    // Basic field initialization
    for (int peerId : peers_) {
        if (peerId == me_) continue;
        nextIndex_[peerId] = getLastLogIndexLocked() + 1; // next log entry to send
        matchIndex_[peerId] = 0; // last known replicated index
    }   
    // Basic invariant checks
    // make sure the local id exists in the peer set (or at least warn).
    bool found_me = false;
    for (int p : peers_) {
        if (p == me_) { found_me = true; break; }
    }
    if (!found_me) {
        spdlog::warn("[Raft] constructor: my id not found in peers vector,continuing", me_);
    }

    persister_ = std::make_shared<Persister>();
    // Register RPC handlers that accept a serialized payload and return a serialized reply.
    // The lambdas: decode -> call local handler function -> encode reply.
    transport_->RegisterRequestVoteHandler(
        [this](const std::string& payload) -> std::string {
            try {
                type::RequestVoteArgs args = codec::RaftCodec::decodeRequestVoteArgs(payload);
                type::RequestVoteReply reply = this->HandleRequestVote(args);
                return codec::RaftCodec::encode(reply);
            } catch (const std::exception& e) {
                spdlog::error("[Raft] {} RequestVote handler exception: {}", this->me_, e.what());
                return std::string();
            }
        }
    );
    transport_->RegisterAppendEntriesHandler(
        [this](const std::string& payload) -> std::string {
            try {
                type::AppendEntriesArgs args = codec::RaftCodec::decodeAppendEntriesArgs(payload);
                type::AppendEntriesReply reply = this->HandleAppendEntries(args);
                return codec::RaftCodec::encode(reply);
            } catch (const std::exception& e) {
                spdlog::error("[Raft] {} AppendEntries handler exception: {}", this->me_, e.what());
                return std::string();
            }
        }
    );

    // Initialize timers
    timerFactory_ = std::make_shared<ThreadTimerFactory>();
    electionTimer_ = timerFactory_->CreateTimer([this]() {this->onElectionTimeout();});
    heartbeatTimer_ = timerFactory_->CreateTimer([this]() {this->onHeartbeatTimeout();});
}

Raft::~Raft() {
    //make sure stop first,join second
    Stop();
    Join();
}
void Raft::SetApplyCallback(std::function<void(type::ApplyMsg&)> cb){
    std::lock_guard<std::mutex> lock(mu_);
    applyCallback_ = std::move(cb);
}

bool Raft::SubmitCommand(const std::string& command){
    std::lock_guard<std::mutex> lock(mu_);
    if (role_.load() != type::Role::Leader) {
        spdlog::warn("[Raft] {} rejected client command '{}': not leader", me_, command);
        return false;
    }
    AppendLogEntryLocked(command);
    // After appending a new log entry, try to replicate it to followers
    broadcastAppendEntriesLocked();
    return true;
}

void Raft::GetState(int32_t& currentTerm, bool& isLeader) const {
    std::lock_guard<std::mutex> lock(mu_);
    currentTerm = currentTerm_;
    isLeader = (role_.load() == type::Role::Leader);
}

void Raft::Start() {
    bool expected = false;
    // parameter: first is the expected value of running_
    // second is the desired value to set
    // returns true if equals to expected and sets to desired 
    // returns false if not equal to expected and does not set
    if (!running_.compare_exchange_strong(expected, true)) {
        spdlog::warn("[Raft] {} Start() called but Raft is already running", me_);
        // already started
        return;
    }
    // start communication layer
    transport_->Start();
    // spdlog::info("[Raft] {} started", me_);
    thread_ = std::thread([this]() { this->run(); });
    apply_thread_ = std::thread([this] { ApplyLoop();});
    // start election timer
    // do not start heartbeat timer yet cuz only leader uses it
    {
        std::lock_guard<std::mutex> lock(mu_);
        resetElectionTimerLocked();
    }
}  

void Raft::Stop(){
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        spdlog::warn("[Raft] {} Stop() called but Raft is not running", me_);
        // already stopped or never started
        return;
    }
    spdlog::info("[Raft] {} Stopping...", me_);
    {
        std::lock_guard<std::mutex> lock(mu_);
        apply_cv_.notify_all();
    }
    electionTimer_->Stop(); 
    heartbeatTimer_->Stop();
    transport_->Stop(); 
    spdlog::info("[Raft] {} Stopped", me_);
}

void Raft::Join() {
    if (thread_.joinable()) {
        // spdlog::info("[Raft] {} Join() - waiting run thread to exit", me_);
        thread_.join();
        // spdlog::info("[Raft] {} Join() - run thread exited", me_);
    }
    if( apply_thread_.joinable()) {
        // spdlog::info("[Raft] {} Join() - waiting apply thread to exit", me_);
        apply_thread_.join();
        // spdlog::info("[Raft] {} Join() - apply thread exited", me_);
    }
}
void Raft::Shutdown(){
    Stop();
    Join();
}
//------------------- Testing utilities -------------------
int32_t Raft::testGetCurrentTerm() const{
    std::lock_guard<std::mutex> lock(mu_);
    return currentTerm_;
}
std::optional<int32_t> Raft::testGetVotedFor() const{
    std::lock_guard<std::mutex> lock(mu_);
    return votedFor_;
}
const std::vector<type::LogEntry>& Raft::testGetLog() const{
    std::lock_guard<std::mutex> lock(mu_);
    return log_;
}
void Raft::testSetCurrentTerm(int32_t term){
    std::lock_guard<std::mutex> lock(mu_);
    currentTerm_ = term;
}
void Raft::testSetVotedFor(std::optional<int32_t> votedFor){
    std::lock_guard<std::mutex> lock(mu_);
    votedFor_ = votedFor;
}
void Raft::testAppendLog(const std::vector<type::LogEntry>& entries){
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& entry : entries) {
        log_.push_back(entry);
        spdlog::info("[Raft] Node {} testAppendLog appended log entry at index {} (term={})", 
                     me_, entry.index, entry.term);
    }
}
void Raft::testSetRaftState(const std::string& state){
    std::lock_guard<std::mutex> lock(mu_);
    persister_->SetRaftState(state);
    readPersistedStateLocked(); // update in-memory state from persisted state
}
type::AppendEntriesReply Raft::testHandleAppendEntries(const type::AppendEntriesArgs& args){
    return HandleAppendEntries(args);
}

std::string Raft::testGetPersistedState() const{
    std::lock_guard<std::mutex> lock(mu_);
    return persister_->GetRaftState();
}

void Raft::testPersistState(){
    std::lock_guard<std::mutex> lock(mu_);
    persistLocked();
}

//-------------------private methods-------------------
void Raft::run() {
    while (running_.load()) {
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
        std::optional<type::RequestVoteReply> reply = sendRequestVoteLocked(peer);
        if(reply){
            if(reply->voteGranted){
                // handle vote granted
                votesGranted++;
                spdlog::info("[Raft] Node {} received vote from node {} (total votes={})", me_, peer, votesGranted);
                // check if won majority
                if (votesGranted >= majority && role_.load() == type::Role::Candidate) {
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
// Rules for 3 types of Nodes
// Follower rules:
// Respond to RPCs from candidates and leaders
// If election timeout elapses without receiving AppendEntries RPC from current leader 
// or granting vote to candidate: convert to candidate
void Raft::becomeFollowerLocked(int32_t newTerm){
    if (role_.load() == type::Role::Follower && currentTerm_ == newTerm) {
        // already follower in this term
        return;
    }
    currentTerm_ = newTerm;
    role_.store(type::Role::Follower);
    spdlog::info("[Raft] Node {} becomes follower (term={})", me_, currentTerm_);
    votedFor_ = std::nullopt;
    persistLocked();
    // stop election timer if running
    // stop heartbeat timer if running
    heartbeatTimer_->Stop();
    // reset election timer
    resetElectionTimerLocked();
}
// Candidate rules:
// On conversion to candidate, start election:
// Increment currentTerm
// Vote for self
// Reset election timer
// Send RequestVote RPCs to all other servers
// If votes received from majority of servers: become leader
// If AppendEntries RPC received from new leader: convert to follower
// If election timeout elapses: start new election
void Raft::becomeCandidateLocked(){
    if (role_.load() == type::Role::Candidate) {
        return; // already candidate
    }
    // Increment current term and convert to candidate
    currentTerm_++;
    role_.store(type::Role::Candidate);
    spdlog::info("[Raft] Node {} becomes candidate (term={})", me_, currentTerm_);
    votedFor_ = me_;
    persistLocked();
    // reset election timer
    resetElectionTimerLocked();
}
// Leader rules:
// 1.Upon election: send initial empty AppendEntries RPCs (heartbeats) to each server;
// repeat during idle periods to prevent election timeouts
// 2. If command received from client: append entry to local log, 
// respond after entry applied to state machine
// 3. If last log index ≥ nextIndex for a follower: send AppendEntries RPC with log entries
// starting at nextIndex
// If successful: update nextIndex and matchIndex for follower
// If AppendEntries fails because of log inconsistency: decrement nextIndex and retry
// 4. If there exists an N such that N > commitIndex, a majority of matchIndex[i] ≥ N,
// and log[N].term == currentTerm: set commitIndex = N
void Raft::becomeLeaderLocked(){
    if (role_.load() == type::Role::Leader) {
        return; // already leader
    }
    role_.store(type::Role::Leader);
    spdlog::info("[Raft] {} becomes Leader (term={})", me_, currentTerm_);
    electionTimer_->Stop();
    //reset to start heartbeat timer
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
    }

    int lastTerm = getLastLogTermLocked();
    int lastIdx = getLastLogIndexLocked();
    // Step 3: vote for candidate only if candidate’s log is at least as up-to-date as receiver’s log
    bool logOk = (args.lastLogTerm > lastTerm) ||
                 (args.lastLogTerm == lastTerm && args.lastLogIndex >= lastIdx);

    //have not voted this term or voted for candidate, and candidate's log is at least as up-to-date
    if ((votedFor_ == std::nullopt || votedFor_ == args.candidateId) && logOk) {
        votedFor_ = args.candidateId;
        persistLocked();
        reply.voteGranted = true;
        resetElectionTimerLocked(); // reset follower timer
        spdlog::info("[Raft] Node {} voted for node {} in term {}", me_, args.candidateId, currentTerm_);
    } else {
        reply.voteGranted = false;
        spdlog::info("[Raft] Node {} rejecting vote for node {} in term {}", me_, args.candidateId, currentTerm_);
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
        spdlog::info("[Raft] {} rejecting AppendeEntries fom {}: stale term ({} < {})",
            me_, args.leaderId, args.term, currentTerm_);
        return reply;
    }
    // Step 2: If term > currentTerm, update and convert to follower
    if (args.term > currentTerm_) {
        becomeFollowerLocked(args.term); 
        spdlog::info("[Raft] Node {} updating term to {} and becomes follower", me_, currentTerm_);
    }
    // Step 3: Reset election timeout since valid leader contacted us
    // Receiving valid AppendEntries acts as heartbeat → reset timeout
    resetElectionTimerLocked();
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
    // If existing entries conflict with new ones (same index but different term), 
    // delete the existing entry and all that follow it
    // spdlog::info("[Raft] {} appending {} entries from leader {}", me_, args.entries.size(), args.leaderId);
    for (size_t i = 0; i < args.entries.size(); ++i) {
        const auto& entry = args.entries[i];
        int index = args.prevLogIndex + 1 + i;
        if (index <= getLastLogIndexLocked()) {
            if(getLogTermLocked(index) != args.entries[i].term){
                // Conflict, delete all entries from index onward
                deleteLogFromIndexLocked(index);

                // Append all remaining entries (including current one)
                for (size_t j = i; j < args.entries.size(); ++j) {
                    log_.push_back(args.entries[j]);
                    spdlog::info("[Raft] Node {} appended new log entry at index {} from leader {}",
                        me_, args.prevLogIndex + 1 + j, args.leaderId);
                }
                break;
            }
        }else{
            log_.push_back(entry);
            spdlog::info("[Raft] Node {} appended new log entry at index {} from leader {}", me_, index, args.leaderId);
        }
    }
    // Step 6: Update commitIndex
    if (args.leaderCommit > commitIndex_) {
        // Only advance commitIndex up to the highest log index we have; 
        // ensures we only commit entries that have been received locally.
        commitIndex_ = std::min(args.leaderCommit, getLastLogIndexLocked());
        // applyLogsLocked(); // apply committed logs to state machine
        apply_cv_.notify_one(); // notify apply thread
        // spdlog::info("[Raft] {} apply_cv_ notified",me_);
    }
    reply.success = true;
    persistLocked();
    return reply;
}

//---------- Log management -----------
void Raft::AppendLogEntryLocked(const std::string& command) {
    type::LogEntry entry;
    entry.term = currentTerm_;
    entry.command = command;
    entry.index = getLastLogIndexLocked() + 1;

    log_.push_back(entry);
    persistLocked();
    spdlog::info("[Raft] Node {} appended new log entry for command '{}'", me_, command);
}



void Raft::ApplyLoop() {
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(mu_);
        apply_cv_.wait(lock, [&] {
            // Wake up if there are committed logs to apply or if stopping
            return lastApplied_ < commitIndex_ || !running_.load();
        });
        if (!running_.load()) break;
        //TODO: apply committed logs to state machine asynchronously
        applyLogsLocked();
    }
}
/*
If there exists an N such that N > commitIndex, 
a majority of matchIndex[i] ≥ N, 
and log[N].term == currentTerm, set commitIndex = N.
*/
void Raft::updateCommitIndexLocked(){
    // Only leader updates commitIndex_
    if (role_.load() != type::Role::Leader) return;

    for (int N = getLastLogIndexLocked(); N > commitIndex_; --N) {
        // Raft §5.4.2: Only commit logs from the current term
        if (getLogTermLocked(N) != currentTerm_) continue;

        int replicatedCount = 1; // count self
        for (const auto& peer : peers_) {
            if (peer == me_) continue;
            if (matchIndex_.at(peer) >= N) {
                replicatedCount++;
            }
        }
        int majority = (peers_.size() / 2) + 1;
        if (replicatedCount >= majority) {
            commitIndex_ = N;
            spdlog::info("[Raft] Node {} updated commitIndex to {}", me_, commitIndex_);
            apply_cv_.notify_one(); // notify apply thread
            break;
        }
    }
}

inline int Raft::getLastLogIndexLocked() const{
    // If log is empty, return 0 , first log index as 1
    return static_cast<int>(log_.size());
}
inline int Raft::getLastLogTermLocked() const{
    // If log is empty, return 0 as initial term
    return log_.empty() ? 0 : log_.back().term;
}
inline int Raft::getLogTermLocked(int index) const{
    // Log index starts at 1 in Raft, so adjust for 0-based vector
    if (index <= 0 || index > static_cast<int>(log_.size())) {
        // Out of range; return 0 as default term
        return 0;
    }
    return log_[index - 1].term;
}
// Returns the log index immediately before what should be sent to the given peer.
inline int Raft::getPrevLogIndexLocked(int peerId) const {
    return nextIndex_.at(peerId) - 1;
}

// Returns the term of the log entry immediately before what should be sent to the given peer.
inline int Raft::getPrevLogTermLocked(int peerId) const {
    int prevIndex = getPrevLogIndexLocked(peerId);
    return prevIndex > 0 ? getLogTermLocked(prevIndex) : 0;
}
// Returns the log entries to send to the given peer for AppendEntries RPC.
std::vector<type::LogEntry> Raft::getEntriesToSendLocked(int peerId) const {
    int start = nextIndex_.at(peerId);
    if (start > getLastLogIndexLocked()) return {};
    int startOffset = start - 1; // adjust for 0-based index
    return std::vector<type::LogEntry>(log_.begin() + startOffset, log_.end());
}

void Raft::applyLogsLocked(){
    // Apply all committed entries not yet applied
    while (lastApplied_ < commitIndex_) {
        lastApplied_++;
        //0 based index for log_ vector
        const type::LogEntry& entry = log_.at(lastApplied_-1);
        // Here should actually apply entry.command to state machine.
        if(applyCallback_){
            type::ApplyMsg msg{
                .CommandValid = true,
                .Command = entry.command,
                .CommandIndex = entry.index
            };
            applyCallback_(msg);
        }
        spdlog::info("[Raft] Node {} applied log entry at index {} with command '{}' (term={})", 
                     me_, lastApplied_, entry.command,entry.term);
    }
}
void Raft::deleteLogFromIndexLocked(int index){
    if (index <= 0 || index > static_cast<int>(log_.size())) {
        spdlog::warn("[Raft] deleteLogFromIndex Invalid index {}, log size={}", index, log_.size());
        return;
    }
    // Erase all entries from `index` to end
    log_.erase(log_.begin() + (index - 1), log_.end());
    spdlog::info("[Raft] deleteLogFromIndex Deleted logs from index {} to end, remaining size={}", 
                 index, log_.size());
}

//--------- Helper functions ----------      
// RVO ensures that returning the struct avoids any unnecessary copies.
std::optional<type::RequestVoteReply> Raft::sendRequestVoteLocked(int peerId){
    // spdlog::info("[Raft] {} sending RequestVote RPC to peer {}", me_, peerId);
    type::RequestVoteArgs args{};
    args.term = currentTerm_;
    args.candidateId = me_;
    args.lastLogIndex = getLastLogIndexLocked();
    args.lastLogTerm = getLastLogTermLocked();

    type::RequestVoteReply reply{};
    bool success = transport_->RequestVoteRPC(peerId, args,reply);

    if (!success) {
        spdlog::warn("[Raft] Failed to send RequestVote RPC to peer {}", peerId);
        return std::nullopt;
    }
    return reply;
} 
std::optional<type::AppendEntriesReply> Raft::sendAppendEntriesLocked(int peerId){
    // spdlog::info("[Raft] {} sending AppendEntries RPC to peer {}", me_, peerId);
    type::AppendEntriesArgs args{};
    args.term = currentTerm_;
    args.leaderId = me_;
    args.prevLogIndex = getPrevLogIndexLocked(peerId);
    args.prevLogTerm = getPrevLogTermLocked(peerId);
    args.entries = getEntriesToSendLocked(peerId);
    args.leaderCommit = commitIndex_;
    type::AppendEntriesReply reply{};
    bool success = transport_->AppendEntriesRPC(peerId, args, reply);

    if (!success) {
        spdlog::warn("[Raft] Failed to send AppendEntries RPC to peer {}", peerId);
        return std::nullopt;
    }
    return reply;
}
void Raft::broadcastHeartbeatLocked(){
    for(const auto& peer : peers_){
        if(peer == me_) continue; // skip self
        std::optional<type::AppendEntriesReply> reply = sendHeartbeatLocked(peer);
        if (reply) {
            if (reply->term > currentTerm_) {
                becomeFollowerLocked(reply->term);
                spdlog::info("[Raft] Node {} stepping down to Follower due to higher term from {}", me_, peer);
                break;
            }
            else {
                // spdlog::info("[Raft] Node {} heartbeat acknowledged by node {}", me_, peer);
            }
        } else {
            spdlog::warn("[Raft] Node {} heartbeat to {} failed", me_, peer);
            //TODO: handle no reply (network failure, timeout)
        }
    }
}
void Raft::broadcastAppendEntriesLocked(){
    for(const auto& peer : peers_){
        if(peer == me_) continue; // skip self
        std::optional<type::AppendEntriesReply> reply = sendAppendEntriesLocked(peer);
        if (reply) {
            //rpc reply received
            if (reply->term > currentTerm_) {
                becomeFollowerLocked(reply->term);
                spdlog::info("[Raft] Node {} stepping down to Follower due to higher term from {}", me_, peer);
                break;
            }
            else {
                if(reply->success){
                    nextIndex_[peer] = getLastLogIndexLocked() + 1;
                    matchIndex_[peer] = nextIndex_[peer] - 1;
                    spdlog::info("[Raft] Node {} AppendEntries success from {}, matchIndex={}, nextIndex={}", 
                    me_, peer, matchIndex_[peer], nextIndex_[peer]);
                    updateCommitIndexLocked();
                } else {
                    // Decrement nextIndex_ on failure
                    nextIndex_[peer] = std::max(1, nextIndex_[peer] - 1);
                    spdlog::info("[Raft] Node {} AppendEntries failed from {}, decrementing nextIndex to {}", 
                        me_, peer, nextIndex_[peer]);
                }
            }
        } else {
            spdlog::warn("[Raft] Node {} AppendEntries to {} failed", me_, peer);
            //TODO: handle no reply (network failure, timeout)
        }
    }
}
void Raft::resetElectionTimerLocked(){
    // Reset election timer with a new randomized timeout
    static thread_local std::mt19937_64 rng(std::random_device{}());
    using Rep = std::chrono::milliseconds::rep;
    Rep low = Raft::ELECTION_TIMEOUT_MIN.count();
    Rep high = Raft::ELECTION_TIMEOUT_MAX.count();
    std::uniform_int_distribution<int> dist(low,high);
    int timeout = dist(rng);
    electionTimer_->Reset(std::chrono::milliseconds(timeout));
}
// Start or restart heartbeat timer
void Raft::resetHeartbeatTimerLocked(){
    heartbeatTimer_->Reset(Raft::HEARTBEAT_INTERVAL);
}
// ----------- Persistent state management -----------
void Raft::persistLocked(){
    persister_->SaveRaftState(currentTerm_, votedFor_, log_);
}
std::string Raft::readPersistedStateLocked(){
    return persister_->ReadRaftState(currentTerm_, votedFor_, log_);
}
// -------- Timer functions -----------
// Called when the election timer times out.
void Raft::onElectionTimeout(){
    std::lock_guard<std::mutex> lock(mu_);
    spdlog::info("[Raft] Election timeout occurred on node {}.", me_);
    becomeCandidateLocked();
    startElectionLocked();
}
// Called when the heartbeat timer times out.
void Raft::onHeartbeatTimeout(){
    std::lock_guard<std::mutex> lock(mu_);
    // spdlog::info("[Raft] Heartbeat timeout occurred on node {}.", me_);
    if (role_.load() != type::Role::Leader) {
        spdlog::error("[Raft] Heartbeat timeout, but node {} is not the leader.", me_);
        return;
    }
    else{
        broadcastHeartbeatLocked();
        resetHeartbeatTimerLocked();
    }
}
std::optional<type::AppendEntriesReply> Raft::sendHeartbeatLocked(int peerId){
    // spdlog::info("[Raft] {} Sending heartbeat AppendEntries to peer {}.", me_,peerId);
    type::AppendEntriesArgs args{};
    args.term = currentTerm_;
    args.leaderId = me_;
    //plz make sure correct function called god damn it lol
    args.prevLogIndex = getPrevLogIndexLocked(peerId);
    args.prevLogTerm = getPrevLogTermLocked(peerId);
    args.entries = {}; // empty for heartbeat
    args.leaderCommit = commitIndex_;

    // spdlog::info("[Raft] Heartbeat AppendEntries args: term={}, leaderId={}, prevLogIndex={}, prevLogTerm={}, leaderCommit={}",
    //     args.term, args.leaderId, args.prevLogIndex, args.prevLogTerm, args.leaderCommit);

    type::AppendEntriesReply reply{};
    bool success = transport_->AppendEntriesRPC(peerId, args, reply);

    if (!success) {
        spdlog::warn("[Raft] Failed to send heartbeat AppendEntries RPC to peer {}", peerId);
        return std::nullopt;
    }
    return reply;
}
}// namespace raft