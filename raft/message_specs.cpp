#include "message_specs.h"

namespace distsysenv {

namespace peers_list {

Bytes RequestPayload::Serialize() const {
  return BuildPayload(peers);
}

RequestPayload RequestPayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  size_t count;
  read_field(bytes, offset, count);

  std::vector<NodeID> peers;
  peers.reserve(count);

  for (size_t i = 0; i < count; ++i) {
    peers.push_back(NodeID::Deserialize(bytes, offset));
  }

  return RequestPayload{.peers = std::move(peers)};
}

}  // namespace peers_list

namespace request_vote {

Bytes RequestPayload::Serialize() const {
  return BuildPayload(term, candidate_id, last_log_index, last_log_term);
}

RequestPayload RequestPayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  TIndex term{};
  read_field(bytes, offset, term);
  NodeID candidate_id = NodeID::Deserialize(bytes, offset);
  TIndex last_log_index{};
  TIndex last_log_term{};

  read_field(bytes, offset, last_log_index);
  read_field(bytes, offset, last_log_term);

  return RequestPayload{
      .term = term,
      .candidate_id = candidate_id,
      .last_log_index = last_log_index,
      .last_log_term = last_log_term,
  };
}

Bytes ResponsePayload::Serialize() const {
  return BuildPayload(term, is_ack);
}

ResponsePayload ResponsePayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  TIndex term{};
  TIndex is_ack{};
  read_field(bytes, offset, term);
  read_field(bytes, offset, is_ack);

  return ResponsePayload{.term = term, .is_ack = is_ack};
}

}  // namespace request_vote

namespace append_entries {

Bytes RequestPayload::Serialize() const {
  Bytes buf;
  append(buf, term, leader_id, last_log_index, last_log_term);

  TIndex n = static_cast<TIndex>(log_entries.size());
  append_item(buf, n);

  for (const LogEntry& e : log_entries) {
    append_item(buf, e.term);

    append_item(buf, e.command);
  }

  append_item(buf, leader_commit_index);

  return buf;
}

RequestPayload RequestPayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  TIndex term;
  read_field(bytes, offset, term);

  NodeID leader_id = NodeID::Deserialize(bytes, offset);

  TIndex last_log_index{};
  TIndex last_log_term{};
  TIndex num_entries{};

  read_field(bytes, offset, last_log_index);
  read_field(bytes, offset, last_log_term);
  read_field(bytes, offset, num_entries);

  std::vector<LogEntry> log_entries;
  if (num_entries > 0) {
    log_entries.reserve(static_cast<size_t>(num_entries));
  }

  for (TIndex i = 0; i < num_entries; ++i) {
    LogEntry entry;
    read_field(bytes, offset, entry.term);
    read_field(bytes, offset, entry.command);

    log_entries.push_back(std::move(entry));
  }

  TIndex leader_commit_index{};
  read_field(bytes, offset, leader_commit_index);

  return RequestPayload{
      .term = term,
      .leader_id = leader_id,
      .last_log_index = last_log_index,
      .last_log_term = last_log_term,
      .log_entries = std::move(log_entries),
      .leader_commit_index = leader_commit_index,
  };
}

Bytes ResponsePayload::Serialize() const {
  return BuildPayload(status, term, match_index);
}

ResponsePayload ResponsePayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  Status status{};
  TIndex term{};
  TIndex match_index{};

  read_field(bytes, offset, status);
  read_field(bytes, offset, term);
  read_field(bytes, offset, match_index);

  return ResponsePayload{
      .status = status,
      .term = term,
      .match_index = match_index,
  };
}

}  // namespace append_entries

namespace command {

Bytes ResponsePayload::Serialize() const {
  return BuildPayload(status, command, value);
}

ResponsePayload ResponsePayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;
  ResponsePayload resp_payload;

  read_field(bytes, offset, resp_payload.status);
  read_field(bytes, offset, resp_payload.command);
  read_field(bytes, offset, resp_payload.value);

  return resp_payload;
}

}  // namespace command

namespace client_redirect {

Bytes RequestPayload::Serialize() const {
  Bytes b;

  append_item(b, reply_to);
  append_item(b, command);

  return b;
}

RequestPayload RequestPayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  const NodeID reply_to = NodeID::Deserialize(bytes, offset);
  TCommand command{};
  read_field(bytes, offset, command);

  return RequestPayload{.reply_to = reply_to, .command = std::move(command)};
}

}  // namespace client_redirect

}  // namespace distsysenv
