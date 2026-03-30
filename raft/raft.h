#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../kv/kv_store.h"
#include "../src/core/event/event.h"
#include "../src/core/message/message.h"
#include "../src/core/node_id/node_id.h"
#include "../src/simulation/context/context.h"
#include "../src/utils/random.h"
#include "message_specs.h"

namespace distsysenv {

class RaftNode {
 public:
  enum class Role { kFOLLOWER, kCANDIDATE, kLEADER };

  RaftNode(std::vector<NodeID> peers);

  void SetPeers(std::vector<NodeID> new_peers);

  void OnSimulationEvent(const Event& e, SimulationContext& ctx);

  void SubmitCommand(const TCommand& command, SimulationContext& ctx);

  TIndex GetLastLogTerm();
  TIndex GetLastLogIndex();

 private:
  void OnMessage(NodeID from, const Message& msg, SimulationContext& ctx);
  void OnLocalMessage(const Message& msg, SimulationContext& ctx);
  void OnTimer(const std::string& timer_name, SimulationContext& ctx);

  void HandleRequestVote(NodeID from, const Message& msg,
                         SimulationContext& ctx);
  void HandleRequestVoteResponse(NodeID from, const Message& msg,
                                 SimulationContext& ctx);
  void HandleAppendEntries(NodeID from, const Message& msg,
                           SimulationContext& ctx);

  void SendAppendEntries(NodeID peer, SimulationContext& ctx);
  void UpdateCommitIndex(SimulationContext& ctx);

  enum class DrainMode : uint8_t {
    kUpToCommitIndex,
    kAll,
  };
  void DrainPendingClientResponses(SimulationContext& ctx, Status response_status,
                                   DrainMode mode);
  void HandleAppendEntriesResponse(NodeID from, const Message& msg,
                                   SimulationContext& ctx);
  void HandleClientCommandRedirected(NodeID from, const Message& msg,
                                     SimulationContext& ctx);
  void HandleClientCommandRedirectedResponse(NodeID from, const Message& msg,
                                             SimulationContext& ctx);

  void ForwardCommandToLeader(const TCommand& command, SimulationContext& ctx);
  void FlushPendingClientCommands(SimulationContext& ctx);
  void ExecuteClientCommand(const TCommand& command, SimulationContext& ctx,
                            std::optional<NodeID> redirect_reply_to);

  void ApplyCommand(TCommand command);
  void ApplyCommittedEntries();

  void BecomeFollower(int new_term, SimulationContext& ctx);
  void BecomeCandidate(SimulationContext& ctx);
  void BecomeLeader(SimulationContext& ctx);
  void StartElection(SimulationContext& ctx);
  bool HasElectionMajority() const;
  void ResetElectionTimer(SimulationContext& ctx);

  void SendHeartbeats(SimulationContext& ctx);

  enum class RaftEvent : uint8_t {
    kBECOME_FOLLOWER,
    kBECOME_CANDIDATE,
    kBECOME_LEADER
  };
  void SendStateToChecker(SimulationContext& ctx, const RaftEvent& raft_event);

  struct PendingClientResponse {
    TIndex log_index;
    std::optional<NodeID> redirect_to;
    TCommand command;
  };

  static constexpr TTime kHEARTBEAT_INTERVAL = 50;
  static constexpr TTime kELECTION_TIMEOUT_MIN = kHEARTBEAT_INTERVAL * 2;
  static constexpr TTime kELECTION_TIMEOUT_MAX = kHEARTBEAT_INTERVAL * 4;

  std::vector<NodeID> peers_;
  std::optional<Random> election_rng_;
  KVStore<TKey, TVal> kv_store_;

  Role role_ = Role::kFOLLOWER;
  TIndex current_term_ = 0;
  std::optional<NodeID> voted_for_;
  std::optional<NodeID> leader_id_;

  std::vector<LogEntry> log_;

  std::unordered_map<NodeID, TIndex> next_index_;
  std::unordered_map<NodeID, TIndex> match_index_;

  TIndex commit_index_ = -1;
  TIndex last_applied_ = -1;

  TIndex votes_received_ = 0;
  std::unordered_set<NodeID> vote_ack_peers_;

  std::deque<TCommand> pending_client_commands_;
  std::deque<PendingClientResponse> pending_client_responses_;
};

}  // namespace distsysenv
