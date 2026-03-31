#include "hybrid_raft.h"

#include <algorithm>
#include <vector>

namespace distsysenv {

void HybridRaftNode::HandleAppendEntries(NodeID from, const Message& msg,
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

  const TIndex last_new = GetLastLogIndex();

  if (req_payload.leader_commit_index > commit_index_) {
    const TIndex new_commit =
        std::min(req_payload.leader_commit_index, last_new);

    if (new_commit > commit_index_) {
      commit_index_ = new_commit;

      ApplyCommittedEntries();
    }
  }

  const TIndex match = GetLastLogIndex();
  append_entries::ResponsePayload resp_payload{
      .status = Status::OK, .term = current_term_, .match_index = match};

  ctx.SendMessage(from, Message::FromDescription("AppendEntriesResponse",
                                                 resp_payload.Serialize()));

  FlushPendingClientCommands(ctx);
}

void HybridRaftNode::SendAppendEntries(NodeID peer, SimulationContext& ctx) {
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

void HybridRaftNode::HandleAppendEntriesResponse(NodeID from,
                                                 const Message& msg,
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

    ctx.SendLocal(
        Message::FromDescription("raft_metric_append_entries_reject", {}));

    SendAppendEntries(from, ctx);

    return;
  }

  match_index_[from] = std::max(match_index_[from], resp_payload.match_index);
  next_index_[from] = match_index_[from] + 1;

  UpdateCommitIndex(ctx);
}

void HybridRaftNode::UpdateCommitIndex(SimulationContext& ctx) {
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

    DrainPendingClientResponses(ctx, Status::OK, DrainMode::kUpToCommitIndex);
  }
}

}  // namespace distsysenv
