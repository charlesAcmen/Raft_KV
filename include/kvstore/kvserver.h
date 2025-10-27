#pragma once 
#include <string>
class KVServer {
public:
    // Public API - exported as RPC handlers
    // Called by network RPC from Clerk
    void PUT(const std::string& key, const std::string& value);
    void APPEND(const std::string& key, const std::string& arg);
    void GET(const std::string& key);
private:

};
