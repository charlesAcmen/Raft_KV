#pragma once

#include <string>
namespace raft {


// Minimal persistence interface. 
class IPersister {
    public:
    virtual ~IPersister() = default;


    // Atomically save Raft persistent state bytes (e.g. currentTerm, votedFor,
    // and optionally log). The exact encoding is left to the implementation.
    virtual void saveState(const std::string& bytes) = 0;


    // Read previously saved state. If no state exists, return an empty string.
    virtual std::string readState() = 0;
};


} // namespace raft