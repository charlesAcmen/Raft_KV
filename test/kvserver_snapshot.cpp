#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "kvstore/kvserver.h"
#include "kvstore/kvcluster.h"
#include "kvstore/statemachine.h"
#include "raft/raft.h"
#include <vector>
using namespace kv;
TEST(KVServer, MaybeSnapshot) {
    spdlog::info("TEST: KVServer maybeTakeSnapshot");

    // -------- 1. init KVCluster --------
    int numServers = 3;
    int numClerks  = 1;
    KVCluster cluster(numServers, numClerks);

    // first KVServer
    std::shared_ptr<KVServer> kvserver = cluster.testGetServer(0);

    kvserver->testSetMaxRaftState(500);

    // -------- 2. fill KV statemachine --------
    std::shared_ptr<KVStateMachine> kvSM = kvserver->testGetSM();
    type::KVCommand cmd1(type::KVCommand::CommandType::PUT, "key1", "value1", 1, 1);
    type::KVCommand cmd2(type::KVCommand::CommandType::PUT, "key2", "value2", 1, 2);
    kvSM->testApply(cmd1.ToString());
    kvSM->testApply(cmd2.ToString());

    // simulate Raft applied log index
    int appliedIndex = 2;

    std::shared_ptr<raft::Raft> rf = kvserver->testGetRaftNode();
    int threshold = kvserver->testGetMaxRaftState();
    // -------- 3. fill Raft logs till surpassing threshold --------
    while (rf->GetPersistSize() < threshold + 1) {
        appliedIndex++;
        rf->testAppendLog({{appliedIndex, 1, "dummy"}});
    }

    // -------- 4. trigger snapshot --------
    kvserver->testMaybeSnapShot(appliedIndex);

    // -------- 5. validate snapshot--------
    std::string snap = kvSM->EncodeSnapShot();
    EXPECT_FALSE(snap.empty()) << "Snapshot should not be empty";

    // -------- 6. validate Raft logs to be compacted --------
    std::vector<raft::type::LogEntry> log = rf->testGetLog();
    EXPECT_GE(log.front().index, appliedIndex)
        << "Log should be compacted, first index >= appliedIndex";

    // -------- 7. validate lastIncludedIndex_ --------
    EXPECT_EQ(kvserver->testGetLastIncludedIndex(), appliedIndex)
        << "lastIncludedIndex_ should match appliedIndex";

    // -------- 8. validate KV statemachine state complete --------
    EXPECT_EQ(kvSM->Get("key1"), "value1");
    EXPECT_EQ(kvSM->Get("key2"), "value2");

    // -------- 9. trigger snapshot again,ensuring not lossing logs--------
    int prevLogSize = log.size();
    kvserver->testMaybeSnapShot(appliedIndex);
    log = rf->testGetLog();
    EXPECT_GE(log.size(), prevLogSize) << "Subsequent snapshot should not remove new logs";

    spdlog::info("TEST: KVServer maybeTakeSnapshot passed");
}

int main(int argc, char **argv) {
    spdlog::set_pattern("[%l] %v");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}