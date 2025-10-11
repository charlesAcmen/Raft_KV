#include "raft/raft.h"
#include "raft/timer.h"
#include "raft/transport.h"
#include "raft/codec/raft_codec.h"
#include <spdlog/spdlog.h>
namespace raft{
Raft::Raft(int me,
    const std::vector<int>& peers,
    std::shared_ptr<IRaftTransport> transport,
    std::function<void(const type::LogEntry&)> applyCallback,
    RaftConfig config,
    std::shared_ptr<ITimerFactory> timerFactory,
    std::shared_ptr<IPersister> persister)
    :me_(me), 
    peers_(peers), 
    transport_(transport),
    applyCallback_(applyCallback), 
    config_(config),
    timerFactory_(timerFactory),
    persister_(persister) {
    // -----------------------
    // Basic invariant checks
    // -----------------------
    // make sure the local id exists in the peer set (or at least warn).
    bool found_me = false;
    for (int p : peers_) {
        if (p == me_) { found_me = true; break; }
    }
    if (!found_me) {
        spdlog::warn("[Raft] constructor: my id not found in peers vector; continuing", me_);
    }

    // -----------------------
    // Register RPC handlers
    // -----------------------
    // If a transport is provided, register the two RPC handlers that accept a serialized
    // payload and return a serialized reply. The transport is responsible for invoking
    // these lambdas when a remote node calls "AppendEntries" / "RequestVote".
    //
    // The lambdas: decode -> call local handler function -> encode reply.
    if (transport_) {
        try {
            transport_->registerRequestVoteHandler(
                [this](const std::string& payload) -> std::string {
                    try {
                        auto args = raft::codec::RaftCodec::decodeRequestVoteArgs(payload);
                        // HandleRequestVote is expected to be a member that returns type::RequestVoteReply
                        auto reply = this->HandleRequestVote(args);
                        return raft::codec::RaftCodec::encode(reply);
                    } catch (const std::exception& e) {
                        spdlog::error("[Raft] {} RequestVote handler exception: {}", this->me_, e.what());
                        return std::string();// caller should check/interpret empty as failure
                    }
                }
            );
            transport_->registerAppendEntriesHandler(
                [this](const std::string& payload) -> std::string {
                    // decode incoming args, run local handler, encode reply
                    try {
                        auto args = raft::codec::RaftCodec::decodeAppendEntriesArgs(payload);
                        // HandleAppendEntries is expected to be a member that returns type::AppendEntriesReply
                        auto reply = this->HandleAppendEntries(args);
                        return raft::codec::RaftCodec::encode(reply);
                    } catch (const std::exception& e) {
                        // on decode/handler error, log and return empty/error payload
                        spdlog::error("[Raft] {} AppendEntries handler exception: {}", this->me_, e.what());
                        return std::string(); 
                    }
                }
            );

            

            // optional — let the transport know which logical raft id this instance is.
            // If IRaftTransport provides a method to register the instance itself, call it here.
            // e.g. transport_->bindRaftInstance(me_, shared_from_this());
            // (Uncomment/adapt if your transport supports it.)
        } catch (const std::exception& e) {
            spdlog::error("[Raft] {} failed to register RPC handlers on transport: {}", me_, e.what());
            // constructor continues; without handlers this node cannot receive RPCs.
        }
    } else {
        spdlog::warn("[Raft] {} constructed without transport (transport_ == nullptr).", me_);
    }

    // -----------------------
    // Other lightweight init (keeps Raft class invariants)
    // -----------------------
    // keep constructor responsibility limited. Heavy initialization (timers,
    // persistent state restore, election timer start) should be done by an explicit Start()
    // or Init() method. Below we only do cheap, safe adjustments.
    //
    // Remove self id from peers_ if accidentally present (so peers_ really means 'other nodes').
    // peers_.erase(std::remove(peers_.begin(), peers_.end(), me_), peers_.end());

    // log constructed state for debugging.
    spdlog::info("[Raft] {} constructed with {} peers (others):", me_, peers_.size());
    for (int p : peers_) spdlog::info("  peer: {}", p);

    // NOTE: do not start timers or mutate other subsystems here if you prefer an explicit Start().
    // If you *do* want automatic start, call Start() or similar here.
}

Raft::~Raft() {
    Kill();
}

void Raft::Kill(){
    
}
}