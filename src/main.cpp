#include "raft/cluster.h"
#include <thread>
#include <chrono>
#include <spdlog/spdlog.h>
int main(){
    spdlog::info("Starting cluster example");
    raft::Cluster cluster;
    cluster.CreateNodes(3);

    // Start all nodes. They will run in background threads.
    cluster.StartAll();

    // (optional) run some short simulation then continue to wait for Ctrl+C
    std::this_thread::sleep_for(std::chrono::seconds(3));
    spdlog::info("3 seconds passed, cluster still running. Press Ctrl+C to stop.");

    // This will block until user presses Ctrl+C (SIGINT),
    // then Cluster will StopAll() + JoinAll() before returning.
    cluster.WaitForShutdown();

    spdlog::info("Cluster stopped, main returns.");
    return 0;
}
