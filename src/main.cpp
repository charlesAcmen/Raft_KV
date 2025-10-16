#include "raft/cluster.h"
#include <spdlog/spdlog.h>
int main(){
    raft::Cluster cluster;
    cluster.CreateNodes(5);

    // Start all nodes. They will run in background threads.
    cluster.StartAll();
    
    // This will block until user presses Ctrl+C (SIGINT),
    // then Cluster will StopAll() + JoinAll() before returning.
    cluster.WaitForShutdown();

    spdlog::info("Cluster stopped, main returns.");
    return 0;
}
