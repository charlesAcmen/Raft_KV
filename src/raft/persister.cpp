#include "raft/persister.h"
#include <sstream>

namespace raft {

// ---------------- Persister ----------------

void Persister::SaveRaftState(const std::string& state) {
    std::lock_guard<std::mutex> lock(mu_);
    raftState_ = state;
}

std::string Persister::ReadRaftState() const {
    std::lock_guard<std::mutex> lock(mu_);
    return raftState_;
}

void Persister::SaveStateAndSnapshot(const std::string& state, const std::string& snapshot) {
    std::lock_guard<std::mutex> lock(mu_);
    raftState_ = state;
    snapshot_ = snapshot;
}

std::string Persister::ReadSnapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
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
} // namespace raft
