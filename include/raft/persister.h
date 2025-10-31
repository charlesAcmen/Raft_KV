#pragma once
#include "raft/types.h"
#include <string>
#include <mutex>
#include <vector>
#include <optional>     //voteFor_
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
    void SaveRaftState(int32_t currentTerm, 
        std::optional<int32_t> votedFor, 
        const std::vector<type::LogEntry>& log);
    // Read back previously saved Raft state
    std::string ReadRaftState(int32_t& currentTerm, 
        std::optional<int32_t>& votedFor,
        std::vector<type::LogEntry>& logData) const;
    // Save both state and snapshot atomically
    void SaveStateAndSnapshot(int32_t currentTerm, 
        std::optional<int32_t> votedFor,
        const std::vector<type::LogEntry>& logData, 
        const std::string& snapshot);
    // Read back previously saved snapshot
    std::string ReadSnapshot() const;
    // Return size (in bytes) of stored state/snapshot
    size_t RaftStateSize() const;
    size_t SnapshotSize() const;
    // For testing: directly set/get raftState_
    void SetRaftState(const std::string& state);
    std::string GetRaftState() const;
private:
    mutable std::mutex mu_;
    std::string raftState_;
    std::string snapshot_;
};//class Persister
} // namespace raft
