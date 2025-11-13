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
    std::uniform_int_distribution<int> key_dist(0, 100);
    int key_num = key_dist(rng);
    // std::string key = "key" + std::to_string(key_num);
    std::string key = "key";
    std::string value = std::to_string(key_num);

    switch (op) {
        case 0:
            clerk->Put(key, value);
            spdlog::info("[main] Put({}, {})", key, value);
            break;
        case 1:
            clerk->Append(key, value);
            spdlog::info("[main] Append({}, {})", key, value);
            break;
        case 2: {
            std::string result = clerk->Get(key);
            spdlog::info("[main] Get({}) -> {}", key, result);
            break;
        }
        default:
            // Should not reach here
            break;
    }
}

void SequentialConsistencyTest(std::vector<std::shared_ptr<kv::Clerk>> clerks) {

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

void ConcurrentPutAppendGetTest(
    // std::shared_ptr<KVCluster> cluster,
    std::shared_ptr<kv::Clerk> clerk1,
    std::shared_ptr<kv::Clerk> clerk2){
    spdlog::info("===== ConcurrentPutAppendGetTest Begin =====");

    const std::string key = "key_concurrent";
    const std::string key2 = "key_isolated";

    // 1️⃣ 测试 ErrNoKey
    {
        std::string result = clerk1->Get(key);
        if (result.empty())
            spdlog::info("[Test] ✅ Correct: Get non-existing key returned empty (ErrNoKey expected)");
        else
            spdlog::error("[Test] ❌ Unexpected value for non-existing key: {}", result);
    }

    // 2️⃣ 并发 Put / Append 同时执行，验证 command 顺序性
    std::thread t1([&] {
        clerk1->Put(key, "v1");
        spdlog::info("[Test] Clerk1 Put({}, v1)", key);
    });

    std::thread t2([&] {
        clerk2->Append(key, "v2");
        spdlog::info("[Test] Clerk2 Append({}, v2)", key);
    });

    t1.join();
    t2.join();

    std::string result = clerk1->Get(key);
    spdlog::info("[Test] Get after concurrent Put+Append -> {}", result);
    if (result == "v1v2" || result == "v2v1")
        spdlog::info("[Test] ✅ Correct: order respected in Raft log, final value consistent");
    else
        spdlog::error("[Test] ❌ Inconsistent result: {}", result);

    // 3️⃣ Leader 切换期间一致性测试
    // spdlog::info("[Test] Forcing leader crash...");
    // cluster->KillLeader();  // 你可以写个 helper，让 cluster 随机停掉当前 leader

    // std::thread t3([&] {
    //     clerk1->Append(key, "_afterCrash");
    // });
    // std::thread t4([&] {
    //     clerk2->Get(key);
    // });
    // t3.join();
    // t4.join();

    // cluster->RecoverAll(); // 重新启动所有节点
    // std::this_thread::sleep_for(std::chrono::seconds(2));

    // std::string final = clerk1->Get(key);
    // spdlog::info("[Test] Final value after leader crash recovery: {}", final);

    // 4️⃣ 测试 key 隔离性
    clerk1->Put(key2, "x1");
    clerk2->Append(key2, "x2");
    std::string res2 = clerk1->Get(key2);
    if (res2 == "x1x2" || res2 == "x2x1")
        spdlog::info("[Test] ✅ Correct: Multiple keys isolated");
    else
        spdlog::error("[Test] ❌ Key isolation failed, got {}", res2);

    spdlog::info("===== ConcurrentPutAppendGetTest End =====");
}
void SequentialAppendTest(
    std::shared_ptr<kv::Clerk> clerk, int operationNum = 10000) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> key_dist(0, 100);

    for (int i = 0; i < operationNum; ++i) {
        int key_num = key_dist(rng);
        // std::string key = "key" + std::to_string(key_num);
        std::string key = "key";
        std::string value = "append" + std::to_string(i); // 保证每次不同

        clerk->Append(key, value);
        // spdlog::info("[main] SequentialAppendTest: Append({}, {})", key, value);
    }

    spdlog::info("[main] SequentialAppendTest finished: {} Append operations done.", operationNum);
}

int main(){
    // kv::KVCluster cluster(5,0);
    int serverNum = 5;
    int clerkNum = 2;
    kv::KVCluster cluster(serverNum,clerkNum);
    cluster.StartAll();

    cluster.WaitForServerLeader(200);
    //--------------------RandomOperation--------------
    // std::shared_ptr<kv::Clerk> clerk = cluster.testGetClerk(0);    
    // std::string key = 
    // // "Grand Theft Auto V"; 
    // "key1";
    // std::string value = 
    // // "Grand Theft Auto VI";
    // "value1";
    // clerk->Put(key,value);

    std::shared_ptr<kv::Clerk> clerk = cluster.testGetClerk(0);
    int operationNum = 100;
    for (int i = 0; i < operationNum; ++i) { RandomClerkOperation(clerk);}

    //--------------------Sequential--------------
    // std::vector<std::shared_ptr<kv::Clerk>> clerks = cluster.testGetClerks();
    // SequentialConsistencyTest(clerks);
    // SequentialAppendTest(clerk, operationNum);


    //--------------------Concurrent--------------
    // std::shared_ptr<kv::Clerk> clerk1 = cluster.testGetClerk(0); 
    // std::shared_ptr<kv::Clerk> clerk2 = cluster.testGetClerk(1); 
    // ConcurrentPutAppendGetTest(clerk1,clerk2);

    cluster.WaitForShutdown();
    return 0;
}