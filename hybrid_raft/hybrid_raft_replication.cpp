#include "hybrid_raft.h"

#include <algorithm>
#include <string>
#include <vector>

#include "hybrid_message_specs.h"

namespace distsysenv {

namespace {

hybrid_msg::append_entries::ResponsePayload MakeConflictHintReject(
    TIndex current_term, const append_entries::RequestPayload& req,
    const std::vector<LogEntry>& log) {
  hybrid_msg::append_entries::ResponsePayload resp{
      .status = Status::ERROR,
      .term = current_term,
      .match_index = -1,
  };

  const TIndex last_log_index = req.last_log_index;
  if (last_log_index < 0) {
    return resp;
  }

  if (last_log_index >= log.size()) {
    resp.conflict_index = log.size();
    return resp;
  }

  const TIndex curr_term = log[last_log_index].term;
  resp.conflict_term = curr_term;

  TIndex idx = last_log_index;
  while (idx > 0 && log[idx - 1].term == curr_term) {
    --idx;
  }

  resp.conflict_index = idx;

  return resp;
}

}  // namespace

void HybridRaftNode::HandleAppendEntries(NodeID from, const Message& msg,
                                         SimulationContext& ctx) {
  append_entries::RequestPayload req_payload =
      append_entries::RequestPayload::Deserialize(msg.GetPayload());
  if (req_payload.term < current_term_) {
    hybrid_msg::append_entries::ResponsePayload resp_payload{
        .status = Status::ERROR, .term = current_term_, .match_index = -1};

    ctx.SendMessage(from,
                    Message::FromDescription("HybridAppendEntriesResponse",
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
      (req_payload.last_log_index < log_.size() &&
       log_[req_payload.last_log_index].term == req_payload.last_log_term);
  if (!log_ok) {
    hybrid_msg::append_entries::ResponsePayload resp =
        MakeConflictHintReject(current_term_, req_payload, log_);
    ctx.SendMessage(from, Message::FromDescription(
                              "HybridAppendEntriesResponse", resp.Serialize()));
    return;
  }

  ResetElectionTimer(ctx);
  RequeueInflightRedirectsNotToLeader(from);
  leader_id_ = from;

  TIndex curr_insert_index = req_payload.last_log_index + 1;

  for (const LogEntry& entry : req_payload.log_entries) {
    if (curr_insert_index < log_.size() &&
        log_[curr_insert_index].term != entry.term) {
      log_.resize(curr_insert_index);
      log_.push_back(entry);

    } else if (curr_insert_index >= log_.size()) {
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
  hybrid_msg::append_entries::ResponsePayload resp_payload{
      .status = Status::OK, .term = current_term_, .match_index = match};

  ctx.SendMessage(from, Message::FromDescription("HybridAppendEntriesResponse",
                                                 resp_payload.Serialize()));

  FlushPendingClientCommands(ctx);
}

void HybridRaftNode::SendAppendEntries(NodeID peer, SimulationContext& ctx) {
  const TIndex max_next = GetLastLogIndex() + 1;
  next_index_[peer] =
      std::clamp(next_index_[peer], static_cast<TIndex>(0), max_next);

  const TIndex prev_index = next_index_[peer] - 1;

  TIndex prev_term = -1;

  if (prev_index >= 0 && prev_index < log_.size()) {
    prev_term = log_[prev_index].term;
  }

  append_entries::RequestPayload req_payload{
      .term = current_term_,
      .leader_id = ctx.GetOwnID(),
      .last_log_index = prev_index,
      .last_log_term = prev_term,
      .log_entries = {},
      .leader_commit_index = commit_index_,
  };

  for (size_t i = next_index_[peer]; i < log_.size(); ++i) {
    req_payload.log_entries.push_back(log_[i]);
  }

  Message msg_to_broadcast =
      Message::FromDescription("AppendEntries", req_payload.Serialize());

  ctx.SendMessage(peer, msg_to_broadcast);
}

void HybridRaftNode::HandleAppendEntriesResponse(NodeID from,
                                                 const Message& msg,
                                                 SimulationContext& ctx) {
  hybrid_msg::append_entries::ResponsePayload resp_payload =
      hybrid_msg::append_entries::ResponsePayload::Deserialize(
          msg.GetPayload());
  if (role_ != Role::kLEADER) {
    return;
  }

  if (resp_payload.term > current_term_) {
    BecomeFollower(resp_payload.term, ctx);
    return;
  }

  if (resp_payload.status == Status::ERROR) {
    ctx.SendLocal(
        Message::FromDescription("raft_metric_append_entries_reject", {}));

    if (resp_payload.conflict_term >= 0) {
      TIndex found = -1;
      for (TIndex i = GetLastLogIndex(); i >= 0; --i) {
        if (log_[i].term == resp_payload.conflict_term) {
          found = i;
          break;
        }
      }

      if (found >= 0) {
        next_index_[from] = found + 1;
      } else if (resp_payload.conflict_index >= 0) {
        next_index_[from] = resp_payload.conflict_index;
      } else {
        next_index_[from] = std::max<TIndex>(0, next_index_[from] - 1);
      }
    } else if (resp_payload.conflict_index >= 0) {
      next_index_[from] = resp_payload.conflict_index;

    } else {
      next_index_[from] = std::max<TIndex>(0, next_index_[from] - 1);
    }

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
