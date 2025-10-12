#include "raft/raft.h"
#include "raft/timer.h"
#include "raft/transport.h"
#include "raft/codec/raft_codec.h"
#include <spdlog/spdlog.h>
namespace raft{
Raft::Raft(int me,
    const std::vector<int>& peers,
    std::shared_ptr<IRaftTransport> transport,
    std::function<void(const type::LogEntry&)> applyCallback,
    RaftConfig config,
    std::shared_ptr<ITimerFactory> timerFactory,
    std::shared_ptr<IPersister> persister)
    :me_(me), 
    peers_(peers), 
    transport_(transport),
    applyCallback_(applyCallback), 
    config_(config),
    timerFactory_(timerFactory),
    persister_(persister) {
    // -----------------------
    // Basic invariant checks
    // -----------------------
    // make sure the local id exists in the peer set (or at least warn).
    bool found_me = false;
    for (int p : peers_) {
        if (p == me_) { found_me = true; break; }
    }
    if (!found_me) {
        spdlog::warn("[Raft] constructor: my id not found in peers vector; continuing", me_);
    }

    // -----------------------
    // Register RPC handlers
    // -----------------------
    // If a transport is provided, register the two RPC handlers that accept a serialized
    // payload and return a serialized reply. The transport is responsible for invoking
    // these lambdas when a remote node calls "AppendEntries" / "RequestVote".
    //
    // The lambdas: decode -> call local handler function -> encode reply.
    if (transport_) {
        try {
            transport_->RegisterRequestVoteHandler(
                [this](const std::string& payload) -> std::string {
                    // decode incoming args, run local handler, encode reply
                    try {
                        type::RequestVoteArgs args = codec::RaftCodec::decodeRequestVoteArgs(payload);
                        // HandleRequestVote is expected to be a member that returns type::RequestVoteReply
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

            

            // optional — let the transport know which logical raft id this instance is.
            // If IRaftTransport provides a method to register the instance itself, call it here.
            // e.g. transport_->bindRaftInstance(me_, shared_from_this());
            // (Uncomment/adapt if your transport supports it.)
        } catch (const std::exception& e) {
            spdlog::error("[Raft] {} failed to register RPC handlers on transport: {}", me_, e.what());
        }
    } else {
        spdlog::warn("[Raft] {} constructed without transport (transport_ == nullptr).", me_);
    }

    // -----------------------
    // Other lightweight init (keeps Raft class invariants)
    // -----------------------
    // keep constructor responsibility limited. Heavy initialization (timers,
    // persistent state restore, election timer start) should be done by an explicit Start()
    // or Init() method. Below we only do cheap, safe adjustments.

    transport_->Start();


    // log constructed state for debugging.
    spdlog::info("[Raft] {} constructed with {} peers", me_, peers_.size());

    // NOTE: do not start timers or mutate other subsystems here if you prefer an explicit Start().
    // If you *do* want automatic start, call Start() or similar here.
}

Raft::~Raft() {
    Kill();
}
std::tuple<int,int,bool> Raft::Start(const std::string& command){
    std::lock_guard<std::mutex> lock(mu_);
    return {-1,-1,false};
}
std::pair<int,bool> Raft::GetState(){
    std::lock_guard<std::mutex> lock(mu_);
    return {currentTerm_, role_ == type::Role::Leader};
}
void Raft::Kill(){
    transport_->Stop();
}
// Handle a RequestVote RPC from a candidate
type::RequestVoteReply Raft::HandleRequestVote(const type::RequestVoteArgs& args){
    std::lock_guard<std::mutex> lock(mu_);
    type::RequestVoteReply reply{};
    
    // Reply's term is always our current term initially
    reply.term = currentTerm_;

    // Step 1: If the term in the request is less than our term, reject
    if (args.term < currentTerm_) {
        reply.voteGranted = false;
        return reply;
    }

    // Step 2: If the term in the request is greater than our term, update term and convert to follower
    if (args.term > currentTerm_) {
        currentTerm_ = args.term;
        votedFor_ = -1; // reset votedFor
        role_ = type::Role::Follower;
    }

    // Step 3: Grant vote if we haven't voted this term and candidate's log is at least as up-to-date
    bool logOk = (args.lastLogTerm > getLastLogTerm()) ||
                 (args.lastLogTerm == getLastLogTerm() && args.lastLogIndex >= getLastLogIndex());

    if ((votedFor_ == -1 || votedFor_ == args.candidateId) && logOk) {
        votedFor_ = args.candidateId;
        reply.voteGranted = true;
        resetElectionTimerLocked(); // reset follower timer
    } else {
        reply.voteGranted = false;
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
        return reply;
    }

    // Step 2: If term > currentTerm, update and convert to follower
    if (args.term > currentTerm_) {
        currentTerm_ = args.term;
        role_ = type::Role::Follower;
        votedFor_ = -1;
    }

    // Step 3: Reset election timeout since valid leader contacted us
    resetElectionTimerLocked();

    // Step 4: Check if log contains entry at prevLogIndex whose term matches prevLogTerm
    if (args.prevLogIndex > getLastLogIndex() ||
        getLogTerm(args.prevLogIndex) != args.prevLogTerm) {
        reply.success = false;
        return reply;
    }

    // Step 5: Append any new entries not already in the log
    // (Here we assume entries contain only sizes or actual entries in a real implementation)
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



type::Role Raft::GetRole(){
    std::lock_guard<std::mutex> lock(mu_);
    return role_;
}
int Raft::GetCurrentTerm(){
    std::lock_guard<std::mutex> lock(mu_);
    return currentTerm_;
}
std::optional<int> Raft::GetVotedFor(){
    std::lock_guard<std::mutex> lock(mu_);
    return votedFor_;
}




//-------------------private methods-------------------
void Raft::startElection(){

}
void Raft::becomeFollower(int32_t newTerm){

}
void Raft::becomeCandidate(){

}
void Raft::becomeLeader(){

}
void Raft::resetElectionTimerLocked(){

}
void Raft::electionTimeoutHandler(){

}


int Raft::getLastLogIndex() const{
    if (log_.empty()) return 0;
    return log_.back().index;
}
int Raft::getLastLogTerm() const{
    if (log_.empty()) return 0;
    return log_.back().term;
}
int Raft::getLogTerm(int index) const{
    if (index == 0) return 0;
    if (index < 1 || index > getLastLogIndex()) {
        throw std::out_of_range("getLogTerm: index out of range");
    }
    return log_[index - 1].term; // log_ is 0-based, index is 1-based
}
void Raft::applyLogs(){
    while (lastApplied_ < commitIndex_) {
        lastApplied_++;
        if (lastApplied_ <= 0 || lastApplied_ > getLastLogIndex()) {
            throw std::out_of_range("applyLogs: lastApplied_ out of range");
        }
        const type::LogEntry& entry = log_[lastApplied_ - 1]; // 1-based to 0-based
        // Apply the log entry to the state machine via callback
        if (applyCallback_) {
            applyCallback_(entry);
        }
    }
}
void Raft::deleteLogFromIndex(int index){
    if (index < 1 || index > getLastLogIndex() + 1) {
        throw std::out_of_range("deleteLogFromIndex: index out of range");
    }
    log_.erase(log_.begin() + (index - 1), log_.end());
}
}// namespace raft