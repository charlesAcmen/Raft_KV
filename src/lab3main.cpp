#include "kvstore/kvcluster.h"
#include "kvstore/clerk.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <chrono>

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



int main(){
    // kv::KVCluster cluster(5,0);
    kv::KVCluster cluster(5,1);
    cluster.StartAll();

    cluster.WaitForServerLeader();
    std::shared_ptr<kv::Clerk> clerk = cluster.testGetClerk(0);    
    // std::string key = 
    // // "Grand Theft Auto V"; 
    // "key1";
    // std::string value = 
    // // "Grand Theft Auto VI";
    // "value1";
    // clerk->Put(key,value);

    int operationNum = 5;
    for (int i = 0; i < operationNum; ++i) { RandomClerkOperation(clerk);}

    cluster.WaitForShutdown();
    
    return 0;
}