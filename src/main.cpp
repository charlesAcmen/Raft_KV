#include "raft/cluster.h"
#include <spdlog/spdlog.h>
void initLogging(){
    // remove timestamp
    //why does it not work?
    spdlog::set_pattern("[%l] %v");
}
int main(){
    initLogging();

    raft::Cluster cluster;
    cluster.CreateNodes(5);

    // Start all nodes. They will run in background threads.
    cluster.StartAll();
    
    

    cluster.WaitForLeader();

   
    cluster.SubmitCommand("SET x 1");
    cluster.SubmitCommand("SET y 2");
    cluster.SubmitCommand("GET x");
    cluster.SubmitCommand("SET x 3");
    cluster.SubmitCommand("GET x");
    cluster.SubmitCommand("INCR y");
    cluster.SubmitCommand("GET y");
    cluster.SubmitCommand("DELETE x");
    cluster.SubmitCommand("GET x");

    // This will block until user presses Ctrl+C (SIGINT),
    // then Cluster will StopAll() + JoinAll() before returning.
    cluster.WaitForShutdown();

    spdlog::info("Cluster stopped, main returns.");
    return 0;
}
