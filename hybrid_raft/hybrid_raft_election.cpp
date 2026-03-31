#include "hybrid_raft.h"

#include <type_traits>

namespace distsysenv {

void HybridRaftNode::HandleRequestVote(NodeID from, const Message& msg,
                                       SimulationContext& ctx) {
  request_vote::RequestPayload req_payload =
      request_vote::RequestPayload::Deserialize(msg.GetPayload());

  if (req_payload.term > current_term_) {
    BecomeFollower(req_payload.term, ctx);
  }

  auto send_vote_response = [&](TIndex is_ack) {
    request_vote::ResponsePayload resp_payload{.term = current_term_,
                                               .is_ack = is_ack};

    ctx.SendMessage(from, Message::FromDescription("RequestVoteResponse",
                                                   resp_payload.Serialize()));
  };

  if (req_payload.candidate_id != from || req_payload.term < current_term_) {
    send_vote_response(0);
    return;
  }

  bool is_log_ok = (req_payload.last_log_term > GetLastLogTerm()) ||
                   (req_payload.last_log_term == GetLastLogTerm() &&
                    req_payload.last_log_index >= GetLastLogIndex());
  bool can_grant = (!voted_for_.has_value() || voted_for_ == from) && is_log_ok;
  if (!can_grant) {
    send_vote_response(0);
    return;
  }

  ResetElectionTimer(ctx);

  voted_for_ = from;
  send_vote_response(1);
}

bool HybridRaftNode::HasElectionMajority() const {
  const size_t cluster = peers_.size() + 1;
  return votes_received_ > cluster / 2;
}

void HybridRaftNode::HandleRequestVoteResponse(NodeID from, const Message& msg,
                                               SimulationContext& ctx) {
  if (role_ != Role::kCANDIDATE) {
    return;
  }

  request_vote::ResponsePayload resp_payload =
      request_vote::ResponsePayload::Deserialize(msg.GetPayload());

  if (resp_payload.term > current_term_) {
    BecomeFollower(resp_payload.term, ctx);
    return;
  }

  if (resp_payload.term != current_term_) {
    return;
  }

  if (resp_payload.is_ack != 0 && vote_ack_peers_.insert(from).second) {
    ++votes_received_;
  }

  if (HasElectionMajority()) {
    BecomeLeader(ctx);
  }
}

void HybridRaftNode::BecomeFollower(int new_term, SimulationContext& ctx) {
  DrainPendingClientResponses(ctx, Status::ERROR, DrainMode::kAll);

  role_ = Role::kFOLLOWER;

  current_term_ = new_term;
  voted_for_ = std::nullopt;
  votes_received_ = 0;
  vote_ack_peers_.clear();
  leader_id_ = std::nullopt;

  ResetElectionTimer(ctx);

  SendStateToChecker(ctx, RaftEvent::kBECOME_FOLLOWER);
}

void HybridRaftNode::BecomeCandidate(SimulationContext& ctx) {
  role_ = Role::kCANDIDATE;

  ++current_term_;
  voted_for_ = ctx.GetOwnID();
  votes_received_ = 1;
  vote_ack_peers_.clear();
  leader_id_ = std::nullopt;

  SendStateToChecker(ctx, RaftEvent::kBECOME_CANDIDATE);
}

void HybridRaftNode::BecomeLeader(SimulationContext& ctx) {
  role_ = Role::kLEADER;

  TIndex next_idx = GetLastLogIndex() + 1;
  for (const auto& peer : peers_) {
    next_index_[peer] = next_idx;
    match_index_[peer] = -1;
  }

  SendHeartbeats(ctx);
  ctx.SetTimer("heartbeat", kHEARTBEAT_INTERVAL);
  ctx.CancelTimer("election");

  SendStateToChecker(ctx, HybridRaftNode::RaftEvent::kBECOME_LEADER);

  FlushPendingClientCommands(ctx);
}

void HybridRaftNode::StartElection(SimulationContext& ctx) {
  BecomeCandidate(ctx);

  request_vote::RequestPayload req_payload{.term = current_term_,
                                           .candidate_id = ctx.GetOwnID(),
                                           .last_log_index = GetLastLogIndex(),
                                           .last_log_term = GetLastLogTerm()};
  Message msg_to_broadcast =
      Message::FromDescription("RequestVote", req_payload.Serialize());

  for (const auto& peer : peers_) {
    ctx.SendMessage(peer, msg_to_broadcast);
  }

  if (HasElectionMajority()) {
    BecomeLeader(ctx);
  } else {
    ResetElectionTimer(ctx);
  }
}

void HybridRaftNode::ResetElectionTimer(SimulationContext& ctx) {
  if (!election_rng_.has_value()) {
    election_rng_.emplace(ctx.GetOwnID().GetHash());
  }

  const TTime spread =
      static_cast<TTime>(GetIndexVal(ctx.GetOwnID().index())) * 40.0f;
  const TTime duration =
      election_rng_->uniform(kELECTION_TIMEOUT_MIN, kELECTION_TIMEOUT_MAX) +
      election_rng_->uniform(TTime{0},
                             kELECTION_TIMEOUT_MAX - kELECTION_TIMEOUT_MIN) +
      spread;

  ctx.SetTimer("election", duration);
}

void HybridRaftNode::SendHeartbeats(SimulationContext& ctx) {
  for (const auto& peer : peers_) {
    SendAppendEntries(peer, ctx);
  }
}

void HybridRaftNode::SendStateToChecker(SimulationContext& ctx,
                                        const RaftEvent& raft_event) {
  Bytes payload(1);
  payload[0] = static_cast<std::byte>(
      static_cast<std::underlying_type_t<RaftEvent>>(raft_event));
  ctx.SendLocal(Message::FromDescription("raft_state", std::move(payload)));
}

}  // namespace distsysenv
