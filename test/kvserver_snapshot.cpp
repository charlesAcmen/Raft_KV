#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "kvstore/kvserver.h"
#include "kvstore/kvcluster.h"
#include "raft/raft.h"
using namespace kv;
TEST(KVServer, MaybeSnapshot) {
    spdlog::info("TEST: KVServer maybeTakeSnapshot");

    // -------- 1. init KVCluster --------
    int numServers = 3;
    int numClerks  = 1;
    KVCluster cluster(numServers, numClerks);

    // first KVServer
    std::shared_ptr<KVServer> kvserver = cluster.testGetServer(0);

    kvserver->testSetMaxRaftState(50);

    // -------- 2. fill KV statemachine --------
    std::shared_ptr<KVStateMachine> kvSM = kvserver->testGetSM();
    KVCommand cmd1(KVCommand::CommandType::PUT, "key1", "value1", 1, 1);
    KVCommand cmd2(KVCommand::CommandType::PUT, "key2", "value2", 1, 2);
    kvSM->testApply(cmd1);
    kvSM->testApply(cmd2);

    // simulate Raft applied log index
    int appliedIndex = 2;

    // -------- 3. 填充 Raft 日志直到超过阈值 --------
    while (kvserver->rf_->GetPersistSize() < kvserver->maxRaftState_ + 1) {
        appliedIndex++;
        kvserver->rf_->testAppendLog({{appliedIndex, 1, "dummy"}});
    }

    // -------- 4. 触发 snapshot --------
    kvserver->testMaybeSnapShot(appliedIndex);

    // -------- 5. 验证 snapshot 已生成 --------
    std::string snap = kvserver->kvSM_->EncodeSnapShot();
    EXPECT_FALSE(snap.empty()) << "Snapshot should not be empty";

    // -------- 6. 验证 Raft 日志被 compact --------
    auto log = kvserver->rf_->testGetLog();
    EXPECT_GE(log.front().index, appliedIndex)
        << "Log should be compacted, first index >= appliedIndex";

    // -------- 7. 验证 lastIncludedIndex_ --------
    EXPECT_EQ(kvserver->lastIncludedIndex_, appliedIndex)
        << "lastIncludedIndex_ should match appliedIndex";

    // -------- 8. 验证 KV 状态机数据完整 --------
    EXPECT_EQ(kvserver->kvSM_->Get("key1"), "value1");
    EXPECT_EQ(kvserver->kvSM_->Get("key2"), "value2");

    // -------- 9. 再次触发 snapshot，确保不会丢失日志 --------
    int prevLogSize = log.size();
    kvserver->testMaybeSnapShot(appliedIndex);
    log = kvserver->rf_->testGetLog();
    EXPECT_GE(log.size(), prevLogSize) << "Subsequent snapshot should not remove new logs";

    spdlog::info("TEST: KVServer maybeTakeSnapshot passed");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}