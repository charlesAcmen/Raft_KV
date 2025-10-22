#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <optional>
#include <vector>
#include <string>
#include "raft/codec/raft_codec.h"
#include "raft/types.h"
#include "raft/persister.h"
#include "raft/raft.h"
#include "raft/transport_unix.h"
using namespace raft::codec;
using namespace raft::type;
using namespace raft;

//-----TEST SUITE FOR RaftCodec::encodeRaftState AND decodeRaftState-----
//param: Test Suite Name: RaftCodecTest, Test Name: EncodeDecodeBasic
TEST(RaftCodecTest, EncodeDecodeBasic) {
    int32_t currentTerm = 3;
    std::optional<int32_t> votedFor = 2;
    std::vector<LogEntry> log = {
        {1, 1, "set x=1"},
        {2, 2, "set y=2"}
    };

    std::string encoded = RaftCodec::encodeRaftState(currentTerm, votedFor, log);

    int32_t decodedTerm;
    std::optional<int32_t> decodedVotedFor;
    std::vector<LogEntry> decodedLog;

    ASSERT_TRUE(RaftCodec::decodeRaftState(encoded, decodedTerm, decodedVotedFor, decodedLog));
    EXPECT_EQ(decodedTerm, currentTerm);
    EXPECT_TRUE(decodedVotedFor.has_value());
    EXPECT_EQ(decodedVotedFor.value(), votedFor.value());
    ASSERT_EQ(decodedLog.size(), log.size());
    for (size_t i = 0; i < log.size(); ++i) {
        EXPECT_EQ(decodedLog[i].index, log[i].index);
        EXPECT_EQ(decodedLog[i].term, log[i].term);
        EXPECT_EQ(decodedLog[i].command, log[i].command);
    }
}
TEST(RaftCodecTest, EncodeDecodeWithNullVotedFor) {
    int32_t currentTerm = 5;
    std::optional<int32_t> votedFor = std::nullopt;
    std::vector<LogEntry> log = {
        {10, 4, "command1"}
    };

    std::string encoded = RaftCodec::encodeRaftState(currentTerm, votedFor, log);

    int32_t decodedTerm;
    std::optional<int32_t> decodedVotedFor;
    std::vector<LogEntry> decodedLog;

    ASSERT_TRUE(RaftCodec::decodeRaftState(encoded, decodedTerm, decodedVotedFor, decodedLog));
    EXPECT_EQ(decodedTerm, currentTerm);
    EXPECT_FALSE(decodedVotedFor.has_value());
    ASSERT_EQ(decodedLog.size(), 1);
    EXPECT_EQ(decodedLog[0].index, 10);
    EXPECT_EQ(decodedLog[0].term, 4);
    EXPECT_EQ(decodedLog[0].command, "command1");
}
TEST(RaftCodecTest, EmptyLogEntries) {
    int32_t currentTerm = 7;
    std::optional<int32_t> votedFor = 1;
    std::vector<LogEntry> log; // empty

    std::string encoded = RaftCodec::encodeRaftState(currentTerm, votedFor, log);

    int32_t decodedTerm;
    std::optional<int32_t> decodedVotedFor;
    std::vector<LogEntry> decodedLog;

    ASSERT_TRUE(RaftCodec::decodeRaftState(encoded, decodedTerm, decodedVotedFor, decodedLog));
    EXPECT_EQ(decodedTerm, currentTerm);
    EXPECT_EQ(decodedVotedFor.value(), votedFor.value());
    EXPECT_TRUE(decodedLog.empty());
}
// TEST(RaftCodecTest, MalformedDataShouldFail) {
//     std::string badData = "abc\nnull\n2\n"; // invalid term
//     int32_t currentTerm;
//     std::optional<int32_t> votedFor;
//     std::vector<LogEntry> log;

//     EXPECT_FALSE(RaftCodec::decodeRaftState(badData, currentTerm, votedFor, log));
// }
TEST(RaftCodecTest, IncompleteLogShouldFail) {
    // Says 2 entries but provides only 1
    std::string data = "1\n1\n2\n1 1 cmd1\n";
    int32_t currentTerm;
    std::optional<int32_t> votedFor;
    std::vector<LogEntry> log;

    EXPECT_FALSE(RaftCodec::decodeRaftState(data, currentTerm, votedFor, log));
}
TEST(RaftCodecTest, EncodeDecodeSpecialCharactersInCommand) {
    int32_t currentTerm = 8;
    std::optional<int32_t> votedFor = 3;
    std::vector<LogEntry> log = {
        {1, 1, "cmd with spaces"},
        {2, 1, "multi_word command  with  spaces"},
        {3, 2, "symbols_!@#$%^&*()"}
    };

    std::string encoded = RaftCodec::encodeRaftState(currentTerm, votedFor, log);

    int32_t decodedTerm;
    std::optional<int32_t> decodedVotedFor;
    std::vector<LogEntry> decodedLog;
    ASSERT_TRUE(RaftCodec::decodeRaftState(encoded, decodedTerm, decodedVotedFor, decodedLog));

    EXPECT_EQ(decodedTerm, currentTerm);
    EXPECT_EQ(decodedVotedFor.value(), votedFor.value());
    ASSERT_EQ(decodedLog.size(), log.size());
    for (size_t i = 0; i < log.size(); ++i) {
        EXPECT_EQ(decodedLog[i].command, log[i].command);
    }
}
//-----TEST SUITE FOR Persister.h-----
//param: Test Suite Name: PersisterTest, Test Name: SaveAndReadBasic
TEST(PersisterTest, SaveAndReadBasic) {
    Persister p;
    std::vector<type::LogEntry> logs = {
        {1, 1, "set x 1"}, {2, 1, "set y 2"}
    };

    p.SaveRaftState(5, 1, logs);

    int32_t term;
    std::optional<int32_t> votedFor;
    std::vector<type::LogEntry> decodedLogs;

    std::string data = p.ReadRaftState(term, votedFor, decodedLogs);

    EXPECT_EQ(term, 5);
    ASSERT_TRUE(votedFor.has_value());
    EXPECT_EQ(votedFor.value(), 1);
    EXPECT_EQ(decodedLogs.size(), logs.size());
    EXPECT_EQ(decodedLogs[0].command, "set x 1");
    EXPECT_EQ(decodedLogs[1].command, "set y 2");
}
// Invalid state should trigger error and return empty string
// TEST(PersisterTest, DecodeFailureShouldReturnEmpty) {
//     Persister p;
//     p.SetRaftState("abc\nnull\n1\n");// invalid numeric term
//     int32_t term;
//     std::optional<int32_t> votedFor;
//     std::vector<type::LogEntry> logs;
//     std::string result = p.ReadRaftState(term, votedFor, logs);
//     EXPECT_EQ(result, "");
//     EXPECT_TRUE(logs.empty());
// }
// Test with null votedFor
TEST(PersisterTest, SaveAndReadWithNullVotedFor) {
    Persister p;
    std::vector<type::LogEntry> logs = {
        {3, 2, "cmd1"}
    };

    p.SaveRaftState(2, std::nullopt, logs);

    int32_t term;
    std::optional<int32_t> votedFor;
    std::vector<type::LogEntry> decodedLogs;

    std::string data = p.ReadRaftState(term, votedFor, decodedLogs);
    EXPECT_EQ(term, 2);
    EXPECT_FALSE(votedFor.has_value());
    EXPECT_EQ(decodedLogs.size(), 1);
    EXPECT_EQ(decodedLogs[0].index, 3);
    EXPECT_EQ(decodedLogs[0].term, 2);
    EXPECT_EQ(decodedLogs[0].command, "cmd1");
}

//-----TEST SUITE FOR RaftPersisterRecovery-----
//param: Test Suite Name: RaftPersisterRecoveryTest, Test Name: RestoreStateAcrossInstances
TEST(RaftPersisterRecoveryTest, RestoreStateAcrossInstances) {
    // -------------------------------
    // Step 1: Initialize Raft instances
    // -------------------------------
    int n = 2;
    std::vector<type::PeerInfo> peers;
    std::vector<int> peerIds;
    for (int i = 0; i < n; ++i) {
        peers.push_back({i+1, "/tmp/raft-node-" + std::to_string(i+1) + ".sock"});
        peerIds.push_back(i+1);
    }

    std::shared_ptr<IRaftTransport> transport1 = 
        std::make_shared<raft::RaftTransportUnix>(peers[0], peers);
    std::shared_ptr<IRaftTransport> transport2 =
        std::make_shared<raft::RaftTransportUnix>(peers[1], peers);
    Raft raft1(peers[0].id, peerIds, transport1);
    Raft raft2(peers[1].id, peerIds, transport2);
    // -------------------------------
    // Step 2: Set some test state in the first instance
    // -------------------------------
    std::vector<type::LogEntry> logEntries = {
        {1, 1, "cmd1"}, {2, 1, "cmd2"}
    };
    raft1.testSetCurrentTerm(5);
    raft1.testSetVotedFor(1);
    raft1.testAppendLog(logEntries);

    // Persist state
    raft1.testPersistState();

    // Save raw string from raft1 to simulate disk/memory
    std::string savedState = raft1.testGetPersistedState();
    spdlog::info("Saved Raft state:{}", savedState);
    // -------------------------------
    // Simulate node crash and restart
    // -------------------------------


    // -------------------------------
    // Step 3: Restore state into raft2 from savedState
    // -------------------------------
    raft2.testSetRaftState(savedState);

    // -------------------------------
    // Step 4: Read state from the new instance and verify
    // -------------------------------
    int32_t term2 = raft2.testGetCurrentTerm();
    std::optional<int32_t> votedFor2 = raft2.testGetVotedFor();
    std::vector<type::LogEntry> log2 = raft2.testGetLog();

    // Verify currentTerm
    EXPECT_EQ(term2, 5) << "CurrentTerm should be restored correctly after simulated restart";

    // Verify votedFor
    ASSERT_TRUE(votedFor2.has_value()) << "votedFor should have a value after restart";
    EXPECT_EQ(votedFor2.value(), 1) << "votedFor value should match persisted state";

    // Verify log entries
    ASSERT_EQ(log2.size(), logEntries.size()) << "Log size should match persisted state";
    for (size_t i = 0; i < logEntries.size(); ++i) {
        EXPECT_EQ(log2[i].index, logEntries[i].index);
        EXPECT_EQ(log2[i].term, logEntries[i].term);
        EXPECT_EQ(log2[i].command, logEntries[i].command);
    }

    // Optional: print log for debug
    for (const auto& entry : log2) {
        spdlog::info("[Raft2] Log entry index={}, term={}, command={}", 
                    entry.index, entry.term, entry.command);
    }

}
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
