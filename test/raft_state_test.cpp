#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <optional>
#include <vector>
#include <string>
#include "raft/codec/raft_codec.h"
#include "raft/types.h"
#include "raft/persister.h"
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
TEST(RaftCodecTest, MalformedDataShouldFail) {
    std::string badData = "abc\nnull\n2\n"; // invalid term
    int32_t currentTerm;
    std::optional<int32_t> votedFor;
    std::vector<LogEntry> log;

    EXPECT_FALSE(RaftCodec::decodeRaftState(badData, currentTerm, votedFor, log));
}
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
TEST(RaftCodecTest, DecodeShouldFailIfNonEmptyLogPassedIn) {
    std::string validData = "1\nnull\n0\n";
    int32_t term;
    std::optional<int32_t> votedFor;
    std::vector<LogEntry> log = {{1, 1, "test"}};

    // decodeRaftState要求logData必须为空
    EXPECT_FALSE(RaftCodec::decodeRaftState(validData, term, votedFor, log));
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
TEST(PersisterTest, DecodeFailureShouldReturnEmpty) {
    Persister p;

    p.SetRaftState("abc\nnull\n1\n");// invalid numeric term

    int32_t term;
    std::optional<int32_t> votedFor;
    std::vector<type::LogEntry> logs;

    std::string result = p.ReadRaftState(term, votedFor, logs);
    EXPECT_EQ(result, "");
    EXPECT_TRUE(logs.empty());
}
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
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
