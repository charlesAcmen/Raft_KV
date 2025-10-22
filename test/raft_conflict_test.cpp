#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "raft/raft.h"
#include "raft/types.h"
#include "raft/transport_unix.h"
#include "raft/timer_thread.h"
TEST(RaftFollower, ConflictDeletion) {
    spdlog::info("Test Raft Follower Conflict Deletion");
    int n = 3;    
    std::vector<raft::type::PeerInfo> peers;
    std::vector<int> peerIds;
    for (int i = 0; i < n; ++i) {
        peers.push_back({i+1, "/tmp/raft-node-" + std::to_string(i+1) + ".sock"});
        peerIds.push_back(i+1);
    }
    raft::type::PeerInfo self = peers[0];
    std::shared_ptr<raft::IRaftTransport> transport = 
            std::make_shared<raft::RaftTransportUnix>(self, peers);
    std::shared_ptr<raft::ITimerFactory> timerFactory = 
        std::make_shared<raft::ThreadTimerFactory>(); 
    raft::Raft follower(self.id,peerIds, transport,timerFactory);
    // 构造 follower 原有日志
    follower.testAppendLog({ {1, 1, "cmd1"} });
    follower.testAppendLog({ {2, 1, "cmd2"} });
    follower.testAppendLog({ {3, 1, "cmd3"} });
    // 构造 leader AppendEntries，有冲突
    raft::type::AppendEntriesArgs args{
        .term = 2,
        .leaderId = 2,
        .prevLogIndex = 0,
        .entries = {
            {1, 1, "cmd1"},
            {2, 2, "cmd2_new"},
            {3, 2, "cmd3_new"}
        },
        .leaderCommit = 3
    };
    // follower 执行 AppendEntries
    follower.testHandleAppendEntries(args);
    auto& log = follower.testGetLog();
    EXPECT_EQ(log.size(), 3);             // 冲突之后旧日志被删除
    EXPECT_EQ(log[0].command, "cmd1");    // 保持不变
    EXPECT_EQ(log[1].command, "cmd2_new");
    EXPECT_EQ(log[2].command, "cmd3_new");
    EXPECT_EQ(log[0].term, 1);
    EXPECT_EQ(log[1].term, 2);
    EXPECT_EQ(log[2].term, 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
