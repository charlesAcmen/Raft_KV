#include "raft/raft.h"
namespace raft{
Raft::Raft(int me,
    const std::vector<int>& peers,
    std::shared_ptr<IRaftTransport> transport,
    std::function<void(const type::LogEntry&)> applyCallback,
    RaftConfig config,
    std::shared_ptr<ITimerFactory> timerFactory,
    std::shared_ptr<IPersister> persister)
    :me_(me), peers_(peers), transport_(transport),
    applyCallback_(applyCallback), config_(config),
    timerFactory_(timerFactory), persister_(persister) {

}
}