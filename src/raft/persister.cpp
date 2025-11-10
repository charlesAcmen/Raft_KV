#include "raft/persister.h"
#include "raft/codec/raft_codec.h"
#include <sstream>
#include <spdlog/spdlog.h>
namespace raft {
void Persister::SaveRaftState(
    int32_t currentTerm, std::optional<int32_t> votedFor, const std::vector<type::LogEntry>& log) {
    std::lock_guard<std::mutex> lock(mu_);
    raftState_ = codec::RaftCodec::encodeRaftState(currentTerm, votedFor, log);
}

std::string Persister::ReadRaftState(
    int32_t& currentTerm, std::optional<int32_t>& votedFor, std::vector<type::LogEntry>& logData) const {
    std::lock_guard<std::mutex> lock(mu_);
    bool success = codec::RaftCodec::decodeRaftState(raftState_,currentTerm, votedFor, logData);
    if(!success){
        spdlog::error("[Persister] Failed to decode Raft state.");
        return "";
    }
    return raftState_;
}

void Persister::SaveStateAndSnapshot(
    int32_t currentTerm, std::optional<int32_t> votedFor,const std::vector<type::LogEntry>& logData, const std::string& snapshot) {
    std::lock_guard<std::mutex> lock(mu_);
    std::string newRaftState = codec::RaftCodec::encodeRaftState(currentTerm, votedFor, logData);
    raftState_ = std::move(newRaftState);
    snapshot_ = snapshot;
}

std::string Persister::ReadSnapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
    if(snapshot_.empty()){
        spdlog::debug("[Persister] ReadSnapshot called but snapshot is empty.");
    }
    return snapshot_;
}

size_t Persister::RaftStateSize() const {
    std::lock_guard<std::mutex> lock(mu_);
    return raftState_.size();
}

size_t Persister::SnapshotSize() const {
    std::lock_guard<std::mutex> lock(mu_);
    return snapshot_.size();
}


//-----------Testing Helpers-----------
void Persister::SetRaftState(const std::string& state) {
    std::lock_guard<std::mutex> lock(mu_);
    raftState_ = state;
}
std::string Persister::GetRaftState() const {
    std::lock_guard<std::mutex> lock(mu_);
    return raftState_;
}
} // namespace raft
