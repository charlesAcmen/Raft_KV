#include <spdlog/spdlog.h>

int main() {
    spdlog::info("Hello from Raft-KV project!");
    spdlog::warn("This is a warning message.");
    spdlog::error("This is an error message.");
    return 0;
}
