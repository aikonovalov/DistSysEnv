#include "raft.h"

#include <cstddef>
#include <optional>
#include <type_traits>
#include "../src/simulation/event/event.h"
#include "message_specs.h"

namespace distsysenv {

RaftNode::RaftNode(std::vector<NodeID> peers) : peers_(std::move(peers)) {}

void RaftNode::SetPeers(std::vector<NodeID> new_peers) {
  peers_ = std::move(new_peers);
}

void RaftNode::OnSimulationEvent(const Event& e, SimulationContext& ctx) {
  SimulationEventKind kind{};
  Status op_status = ClassifySimulationEvent(e, &kind);
  if (op_status != Status::OK) {
    return;
  }

  switch (kind) {
    case SimulationEventKind::Message: {
      DecodedMessageEvent dec{
          .from = ctx.GetOwnID(),
          .to = ctx.GetOwnID(),
          .msg = Message::FromDescription("_", {}),
      };

      op_status = TryDecodeMessageEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      if (dec.status != MessageDeliveryStatus::Received) {
        return;
      }
      if (dec.to != ctx.GetOwnID()) {
        return;
      }

      OnMessage(dec.from, dec.msg, ctx);

      return;
    }
    case SimulationEventKind::LocalMessage: {
      DecodedLocalMessageEvent dec{NodeID(Index{0}, Generation{0}),
                                   Message::FromDescription("_", {})};

      op_status = TryDecodeLocalMessageEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      OnLocalMessage(dec.msg, ctx);

      return;
    }
    case SimulationEventKind::Timer: {
      if (e.to != ctx.GetOwnID()) {
        return;
      }

      DecodedTimerEvent dec{};
      op_status = TryDecodeTimerEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      if (!ctx.IsTimerValid(dec.name, dec.token)) {
        return;
      }

      OnTimer(dec.name, ctx);

      return;
    }
    case SimulationEventKind::NodeStatus:
      return;
  }
}

void RaftNode::OnMessage(NodeID from, const Message& msg,
                         SimulationContext& ctx) {
  MessageType curr_type = msg.GetType();

  if (curr_type == "RequestVote") {
    HandleRequestVote(from, msg, ctx);

  } else if (curr_type == "RequestVoteResponse") {
    HandleRequestVoteResponse(from, msg, ctx);

  } else if (curr_type == "AppendEntries") {
    HandleAppendEntries(from, msg, ctx);

  } else if (curr_type == "AppendEntriesResponse") {
    HandleAppendEntriesResponse(from, msg, ctx);
  } else if (curr_type == "client_command_redirected") {
    HandleClientCommandRedirected(from, msg, ctx);
  } else if (curr_type == "client_command_redirected_response") {
    HandleClientCommandRedirectedResponse(from, msg, ctx);
  }
}

void RaftNode::OnLocalMessage(const Message& msg, SimulationContext& ctx) {
  MessageType curr_type = msg.GetType();

  if (curr_type == "set_peers") {
    SetPeers(peers_list::RequestPayload::Deserialize(msg.GetPayload()).peers);

  } else if (curr_type == "client_command") {
    TOffset offset = 0;
    TCommand command = TCommand::Deserialize(msg.GetPayload(), offset);
    SubmitCommand(command, ctx);

  } else if (curr_type == "start") {
    ResetElectionTimer(ctx);
  }
}

void RaftNode::OnTimer(const std::string& timer_name, SimulationContext& ctx) {
  if (timer_name == "election") {
    StartElection(ctx);

  } else if (timer_name == "heartbeat") {
    if (role_ == Role::kLEADER) {
      SendHeartbeats(ctx);
      ctx.SetTimer("heartbeat", kHEARTBEAT_INTERVAL);
    }
  }
}

void RaftNode::SubmitCommand(const TCommand& command, SimulationContext& ctx) {
  if (role_ != Role::kLEADER) {
    if (leader_id_.has_value()) {
      ForwardCommandToLeader(command, ctx);

    } else {
      pending_client_commands_.push_back(command);
    }
    return;
  }

  ExecuteClientCommand(command, ctx, std::nullopt);
}

void RaftNode::ForwardCommandToLeader(const TCommand& command,
                                      SimulationContext& ctx) {
  if (!leader_id_.has_value()) {
    pending_client_commands_.push_back(command);
    return;
  }

  client_redirect::RequestPayload payload{.reply_to = ctx.GetOwnID(),
                                          .command = command};
  ctx.SendMessage(*leader_id_,
                  Message::FromDescription("client_command_redirected",
                                           payload.Serialize()));
}

void RaftNode::FlushPendingClientCommands(SimulationContext& ctx) {
  if (role_ == Role::kLEADER) {
    while (!pending_client_commands_.empty()) {
      TCommand cmd = std::move(pending_client_commands_.front());
      pending_client_commands_.pop_front();

      ExecuteClientCommand(cmd, ctx, std::nullopt);
    }

    return;
  }

  if (!leader_id_.has_value()) {
    return;
  }

  while (!pending_client_commands_.empty()) {
    TCommand cmd = std::move(pending_client_commands_.front());
    pending_client_commands_.pop_front();
    ForwardCommandToLeader(cmd, ctx);
  }
}

void RaftNode::ExecuteClientCommand(const TCommand& command,
                                    SimulationContext& ctx,
                                    std::optional<NodeID> redirect_to) {
  auto send_response = [&](const command::ResponsePayload& resp_payload) {
    if (redirect_to.has_value()) {
      ctx.SendMessage(*redirect_to, Message::FromDescription(
                                        "client_command_redirected_response",
                                        resp_payload.Serialize()));
    } else {
      ctx.SendLocal(Message::FromDescription("client_command_response",
                                             resp_payload.Serialize()));
    }
  };

  if (command.type() == TCommand::Type::eGET) {
    send_response(command::ResponsePayload{
        .status = Status::OK,
        .command = command,
        .value = kv_store_.Get(command.key()),
    });

    return;
  }

  log_.push_back(LogEntry{.term = current_term_, .command = command});

  for (const auto& peer : peers_) {
    SendAppendEntries(peer, ctx);
  }

  send_response(command::ResponsePayload{
      .status = Status::OK,
      .command = command,
      .value = std::nullopt,
  });
}

TIndex RaftNode::GetLastLogTerm() {
  if (log_.empty()) {
    return -1;
  }

  const LogEntry& last_log_entry = log_.back();

  return last_log_entry.term;
}

TIndex RaftNode::GetLastLogIndex() {
  if (log_.empty()) {
    return -1;
  }

  return log_.size() - 1;
}

void RaftNode::HandleRequestVote(NodeID from, const Message& msg,
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

bool RaftNode::HasElectionMajority() const {
  const size_t cluster = peers_.size() + 1;
  return votes_received_ > cluster / 2;
}

void RaftNode::HandleRequestVoteResponse(NodeID from, const Message& msg,
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

  if (resp_payload.is_ack != 0) {
    ++votes_received_;
  }

  if (HasElectionMajority()) {
    BecomeLeader(ctx);
  }
}

void RaftNode::HandleAppendEntries(NodeID from, const Message& msg,
                                   SimulationContext& ctx) {
  append_entries::RequestPayload req_payload =
      append_entries::RequestPayload::Deserialize(msg.GetPayload());
  if (req_payload.term < current_term_) {
    append_entries::ResponsePayload resp_payload{
        .status = Status::ERROR, .term = current_term_, .match_index = -1};

    ctx.SendMessage(from, Message::FromDescription("AppendEntriesResponse",
                                                   resp_payload.Serialize()));

    return;
  }

  if (req_payload.term > current_term_) {
    BecomeFollower(req_payload.term, ctx);
  } else if (role_ == Role::kCANDIDATE) {
    role_ = Role::kFOLLOWER;
    votes_received_ = 0;
    voted_for_ = std::nullopt;

    ctx.CancelTimer("election");

    ResetElectionTimer(ctx);
  }

  bool log_ok =
      (req_payload.last_log_index == -1) ||
      (req_payload.last_log_index < static_cast<TIndex>(log_.size()) &&
       log_[req_payload.last_log_index].term == req_payload.last_log_term);
  if (!log_ok) {
    append_entries::ResponsePayload resp{
        .status = Status::ERROR, .term = current_term_, .match_index = -1};
    ctx.SendMessage(from, Message::FromDescription("AppendEntriesResponse",
                                                   resp.Serialize()));
    return;
  }

  ResetElectionTimer(ctx);
  leader_id_ = from;

  TIndex curr_insert_index = req_payload.last_log_index + 1;

  for (const LogEntry& entry : req_payload.log_entries) {
    if (curr_insert_index < static_cast<TIndex>(log_.size()) &&
        log_[curr_insert_index].term != entry.term) {
      log_.resize(curr_insert_index);
      log_.push_back(entry);

    } else if (curr_insert_index >= static_cast<TIndex>(log_.size())) {
      log_.push_back(entry);

    } else {
      log_[curr_insert_index] = entry;
    }

    ++curr_insert_index;
  }

  const TIndex match = GetLastLogIndex();
  append_entries::ResponsePayload resp_payload{
      .status = Status::OK, .term = current_term_, .match_index = match};

  ctx.SendMessage(from, Message::FromDescription("AppendEntriesResponse",
                                                 resp_payload.Serialize()));

  FlushPendingClientCommands(ctx);
}

void RaftNode::SendAppendEntries(NodeID peer, SimulationContext& ctx) {
  TIndex prev_index = next_index_[peer] - 1;
  TIndex prev_term =
      (prev_index >= 0) ? log_[prev_index].term : static_cast<TIndex>(-1);

  append_entries::RequestPayload req_payload{
      .term = current_term_,
      .leader_id = ctx.GetOwnID(),
      .last_log_index = prev_index,
      .last_log_term = prev_term,
      .log_entries = {},
      .leader_commit_index = commit_index_,
  };

  for (size_t i = next_index_[peer]; i < static_cast<size_t>(log_.size());
       ++i) {
    req_payload.log_entries.push_back(log_[i]);
  }

  Message msg_to_broadcast =
      Message::FromDescription("AppendEntries", req_payload.Serialize());

  ctx.SendMessage(peer, msg_to_broadcast);
}

void RaftNode::HandleAppendEntriesResponse(NodeID from, const Message& msg,
                                           SimulationContext& ctx) {
  append_entries::ResponsePayload resp_payload =
      append_entries::ResponsePayload::Deserialize(msg.GetPayload());
  if (role_ != Role::kLEADER) {
    return;
  }

  if (resp_payload.term > current_term_) {
    BecomeFollower(resp_payload.term, ctx);
    return;
  }

  if (resp_payload.status == Status::ERROR) {
    next_index_[from] = std::max<TIndex>(0, next_index_[from] - 1);

    SendAppendEntries(from, ctx);

    return;
  }

  match_index_[from] = std::max(match_index_[from], resp_payload.match_index);
  next_index_[from] = match_index_[from] + 1;

  UpdateCommitIndex();
}

void RaftNode::HandleClientCommandRedirected(NodeID from, const Message& msg,
                                             SimulationContext& ctx) {
  (void)from;
  if (role_ != Role::kLEADER) {
    return;
  }

  client_redirect::RequestPayload payload =
      client_redirect::RequestPayload::Deserialize(msg.GetPayload());

  ExecuteClientCommand(payload.command, ctx, payload.reply_to);
}

void RaftNode::HandleClientCommandRedirectedResponse(NodeID from,
                                                     const Message& msg,
                                                     SimulationContext& ctx) {
  (void)from;
  command::ResponsePayload resp_payload =
      command::ResponsePayload::Deserialize(msg.GetPayload());
  ctx.SendLocal(Message::FromDescription("client_command_response",
                                         resp_payload.Serialize()));
}

void RaftNode::UpdateCommitIndex() {
  std::vector<TIndex> match_indexes;
  match_indexes.push_back(GetLastLogIndex());

  for (const auto& [_, val] : match_index_) {
    match_indexes.push_back(val);
  }

  std::sort(match_indexes.begin(), match_indexes.end());

  TIndex candidate = match_indexes[(match_indexes.size() - 1) / 2];

  if (candidate > commit_index_ && log_[candidate].term == current_term_) {
    commit_index_ = candidate;
    ApplyCommittedEntries();
  }
}

void RaftNode::ApplyCommand(TCommand command) {
  switch (command.type()) {
    case TCommand::Type::eGET:
      break;
    case TCommand::Type::eSET:
      if (command.value().has_value()) {
        kv_store_.Set(command.key(), *command.value());
      }

      break;
    case TCommand::Type::eDEL:
      kv_store_.Delete(command.key());

      break;
  }
}

void RaftNode::ApplyCommittedEntries() {
  while (last_applied_ < commit_index_) {
    ++last_applied_;

    TCommand curr_command = log_[last_applied_].command;

    ApplyCommand(curr_command);
  }
}

void RaftNode::BecomeFollower(int new_term, SimulationContext& ctx) {
  role_ = Role::kFOLLOWER;

  current_term_ = new_term;
  voted_for_ = std::nullopt;
  votes_received_ = 0;
  leader_id_ = std::nullopt;

  ResetElectionTimer(ctx);

  SendStateToChecker(ctx, RaftEvent::kBECOME_FOLLOWER);
}

void RaftNode::BecomeCandidate(SimulationContext& ctx) {
  role_ = Role::kCANDIDATE;

  ++current_term_;
  voted_for_ = ctx.GetOwnID();
  votes_received_ = 1;
  leader_id_ = std::nullopt;

  SendStateToChecker(ctx, RaftEvent::kBECOME_CANDIDATE);
}

void RaftNode::BecomeLeader(SimulationContext& ctx) {
  role_ = Role::kLEADER;

  TIndex next_idx = GetLastLogIndex() + 1;
  for (const auto& peer : peers_) {
    next_index_[peer] = next_idx;
    match_index_[peer] = -1;
  }

  SendHeartbeats(ctx);
  ctx.SetTimer("heartbeat", kHEARTBEAT_INTERVAL);
  ctx.CancelTimer("election");

  SendStateToChecker(ctx, RaftNode::RaftEvent::kBECOME_LEADER);

  FlushPendingClientCommands(ctx);
}

void RaftNode::StartElection(SimulationContext& ctx) {
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

void RaftNode::ResetElectionTimer(SimulationContext& ctx) {
  if (!election_rng_.has_value()) {
    election_rng_.emplace(ctx.GetOwnID().GetHash());
  }

  const TTime duration =
      election_rng_->uniform(kELECTION_TIMEOUT_MIN, kELECTION_TIMEOUT_MAX);

  ctx.SetTimer("election", duration);
}

void RaftNode::SendHeartbeats(SimulationContext& ctx) {
  for (const auto& peer : peers_) {
    SendAppendEntries(peer, ctx);
  }
}

void RaftNode::SendStateToChecker(SimulationContext& ctx,
                                  const RaftEvent& raft_event) {
  Bytes payload(1);
  payload[0] = static_cast<std::byte>(
      static_cast<std::underlying_type_t<RaftEvent>>(raft_event));
  ctx.SendLocal(Message::FromDescription("raft_state", std::move(payload)));
}

}  // namespace distsysenv
