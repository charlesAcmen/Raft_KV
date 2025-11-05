#include "kvstore/kvcluster.h"
#include "kvstore/clerk.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <chrono>
#include <unordered_map>
void RandomClerkOperation(std::shared_ptr<kv::Clerk> clerk) {
    // Seed random engine with system clock
    static std::mt19937 rng(std::random_device{}());

    // Randomly pick operation: 0 => Put, 1 => Append, 2 => Get
    std::uniform_int_distribution<int> op_dist(0, 2);
    int op = op_dist(rng);

    // Generate a random key and value
    std::uniform_int_distribution<int> key_dist(0, 1000);
    int key_num = key_dist(rng);
    std::string key = "key" + std::to_string(key_num);
    std::string value = "value" + std::to_string(key_num);

    switch (op) {
        case 0:
            clerk->Put(key, value);
            spdlog::info("[main] RandomClerkOperation: Put({}, {})", key, value);
            break;
        case 1:
            clerk->Append(key, value);
            spdlog::info("[main] RandomClerkOperation: Append({}, {})", key, value);
            break;
        case 2: {
            std::string result = clerk->Get(key);
            spdlog::info("[main] RandomClerkOperation: Get({}) -> {}", key, result);
            break;
        }
        default:
            // Should not reach here
            break;
    }
}

void SequentialConsistencyTest(
    std::vector<std::shared_ptr<kv::Clerk>> clerks) {

    spdlog::info("===== SequentialConsistencyTest Begin =====");

    const std::string key = "key_seq";
    const std::string v1 = "v1";
    const std::string v2 = "v2";

    // 1️⃣ Clerk 0 写入 Put(key_seq, v1)
    clerks[0]->Put(key, v1);
    spdlog::info("[Test] Clerk0 Put({}, {})", key, v1);

    // 2️⃣ Clerk 0 立刻读取 Get(key_seq)
    std::string r1 = clerks[0]->Get(key);
    spdlog::info("[Test] Clerk0 Get({}) -> {}", key, r1);

    // 检查是否等于 v1
    if (r1 != v1) {
        spdlog::error("[Test] ❌ Unexpected Get result after Put: got '{}', expected '{}'", r1, v1);
    } else {
        spdlog::info("[Test] ✅ Correct: Get after Put = {}", r1);
    }

    // 3️⃣ Clerk 1 执行 Append(key_seq, v2)
    clerks[1]->Append(key, v2);
    spdlog::info("[Test] Clerk1 Append({}, {})", key, v2);

    // 4️⃣ Clerk 1 再次读取
    std::string r2 = clerks[1]->Get(key);
    spdlog::info("[Test] Clerk1 Get({}) -> {}", key, r2);

    // 检查是否等于 v1 + v2
    if (r2 != v1 + v2) {
        spdlog::error("[Test] ❌ Unexpected result after Append: got '{}', expected '{}'", r2, v1 + v2);
    } else {
        spdlog::info("[Test] ✅ Correct: Get after Append = {}", r2);
    }

    // 5️⃣ 所有 Clerk 再次读取同一 key，结果应一致
    std::unordered_map<int, std::string> results;
    for (size_t i = 0; i < clerks.size(); ++i) {
        std::string v = clerks[i]->Get(key);
        results[i] = v;
        spdlog::info("[Test] Clerk{} final Get({}) -> {}", i, key, v);
    }

    // 6️⃣ 验证所有结果相同
    bool consistent = true;
    for (size_t i = 1; i < clerks.size(); ++i) {
        if (results[i] != results[0]) {
            consistent = false;
            spdlog::error("[Test] ❌ Inconsistent view: Clerk0='{}', Clerk{}='{}'",
                results[0], i, results[i]);
        }
    }

    if (consistent) {
        spdlog::info("[Test] ✅ All clerks see same value '{}'", results[0]);
    }

    spdlog::info("===== SequentialConsistencyTest End =====");
}


int main(){
    // kv::KVCluster cluster(5,0);
    kv::KVCluster cluster(5,2);
    cluster.StartAll();

    cluster.WaitForServerLeader();
    // std::shared_ptr<kv::Clerk> clerk = cluster.testGetClerk(0);    
    // std::string key = 
    // // "Grand Theft Auto V"; 
    // "key1";
    // std::string value = 
    // // "Grand Theft Auto VI";
    // "value1";
    // clerk->Put(key,value);

    // int operationNum = 10;
    // for (int i = 0; i < operationNum; ++i) { RandomClerkOperation(clerk);}

    std::vector<std::shared_ptr<kv::Clerk>> clerks = cluster.testGetClerks();
    SequentialConsistencyTest(clerks);
    cluster.WaitForShutdown();
    
    return 0;
}