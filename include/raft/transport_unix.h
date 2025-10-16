#pragma once
#include "transport.h" 
#include <vector>
#include <string>
#include <functional>
namespace raft {

// Unix-domain-socket-based transport for single-machine multi-process simulation.
class RaftTransportUnix : public IRaftTransport {
public:
    explicit RaftTransportUnix(const type::PeerInfo&,const std::vector<type::PeerInfo>&);
    ~RaftTransportUnix() override;

    void Start() override;
    void Stop() override;

    bool RequestVoteRPC(int,const type::RequestVoteArgs&,type::RequestVoteReply&) override;
    bool AppendEntriesRPC(int,const type::AppendEntriesArgs&,type::AppendEntriesReply&) override;

    void RegisterRequestVoteHandler(
        std::function<std::string(const std::string&)> handler) override;
    void RegisterAppendEntriesHandler(
        std::function<std::string(const std::string&)> handler) override;
private:
    std::thread serverThread_;
    std::thread clientThread_;
};

} // namespace raft
