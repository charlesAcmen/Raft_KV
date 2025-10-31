#include "raft/timer.h"
namespace raft{
ITimer::ITimer(std::function<void()>& cb)
    : callback_(std::move(cb)) {
}
}// namespace raft