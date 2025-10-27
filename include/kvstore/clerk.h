#pragma once
#include <string>
#include <vector>
#include <memory>   
#include <cstdint>          //for int64_t
#include "kvstore/kvcommand.h"
#include "kvstore/transport.h"
// ---------- clerk.h ----------
class Clerk {
public:
    Clerk();

    // Public API for test harness: names in ALL CAPS
    std::string GET(const std::string& key);
    std::string PUT(const std::string& key, const std::string& value);
    std::string APPEND(const std::string& key, const std::string& arg);

private:
    
};
