#pragma once
#include <string>
#include <mutex>
#include <vector>

namespace raft {

/**
 * @brief Persister is responsible for saving and loading
 *        persistent Raft state and snapshot data.
 * 
 * It simulates stable storage (like disk) for Raft.
 * All methods are thread-safe.
 */
class Persister {
public:
    Persister() = default;
    Persister(const Persister&) = delete;
    Persister& operator=(const Persister&) = delete;

    // Save only the Raft internal state (term, vote, log)
    void SaveRaftState(const std::string& state);

    // Read back previously saved Raft state
    std::string ReadRaftState() const;

    // Save both state and snapshot atomically
    void SaveStateAndSnapshot(const std::string& state, const std::string& snapshot);

    // Read back previously saved snapshot
    std::string ReadSnapshot() const;

    // Return size (in bytes) of stored state/snapshot
    size_t RaftStateSize() const;
    size_t SnapshotSize() const;

private:
    mutable std::mutex mu_;
    std::string raftState_;
    std::string snapshot_;
};
} // namespace raft
