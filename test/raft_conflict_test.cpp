#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
// #include "raft/raft.h"
// #include "raft/types.h"

TEST(RaftFollower, ConflictDeletion) {
    spdlog::info("Test Raft Follower Conflict Deletion");
    raft::Raft follower(1, {1,2,3}, nullptr, nullptr);
    // 构造 follower 原有日志
    follower.testAppendLog({{"cmd1", 1, 1}});
    follower.testAppendLog({{"cmd2", 2, 1}});
    follower.testAppendLog({{"cmd3", 3, 1}});
    // 构造 leader AppendEntries，有冲突
    raft::type::AppendEntriesArgs args;
    args.term = 2;
    args.leaderId = 1;
    args.prevLogIndex = 0; // leader 期望 follower 从 index 1 开始
    args.entries = {
        {"cmd1", 1, 1},       // 保持相同
        {"cmd2_new", 2, 2},   // term 不同，触发冲突删除
        {"cmd3_new", 3, 2}
    };
    args.leaderCommit = 3;
    // follower 执行 AppendEntries
    follower.handleAppendEntries(args);
    auto& log = follower.testGetLog();
    EXPECT_EQ(log.size(), 3);             // 冲突之后旧日志被删除
    EXPECT_EQ(log[0].command, "cmd1");    // 保持不变
    EXPECT_EQ(log[1].command, "cmd2_new");
    EXPECT_EQ(log[2].command, "cmd3_new");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
