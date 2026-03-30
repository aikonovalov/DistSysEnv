#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include "../kv/kv_store.h"
#include "../src/core/node_id/node_id.h"
#include "../src/utils/utils.h"

namespace distsysenv {

using TKey = std::string;
using TVal = std::string;

using TCommand = Command<TKey, TVal>;

struct LogEntry {
  TIndex term;
  TCommand command;
};

namespace peers_list {

struct RequestPayload {
  std::vector<NodeID> peers;

  Bytes Serialize() const;
  static RequestPayload Deserialize(const Bytes& bytes);
};

}  // namespace peers_list

namespace request_vote {

struct RequestPayload {
  TIndex term;
  NodeID candidate_id;
  TIndex last_log_index;
  TIndex last_log_term;

  Bytes Serialize() const;
  static RequestPayload Deserialize(const Bytes& bytes);
};

struct ResponsePayload {
  TIndex term;
  TIndex is_ack;

  Bytes Serialize() const;
  static ResponsePayload Deserialize(const Bytes& bytes);
};

}  // namespace request_vote

namespace append_entries {

struct RequestPayload {
  TIndex term;
  NodeID leader_id;
  TIndex last_log_index;
  TIndex last_log_term;
  std::vector<LogEntry> log_entries;
  TIndex leader_commit_index;

  Bytes Serialize() const;
  static RequestPayload Deserialize(const Bytes& bytes);
};

struct ResponsePayload {
  Status status;
  TIndex term;
  TIndex match_index;

  Bytes Serialize() const;
  static ResponsePayload Deserialize(const Bytes& bytes);
};

}  // namespace append_entries

namespace command {

using RequestPayload = TCommand;

struct ResponsePayload {
  Status status;
  TCommand command;
  std::optional<TVal> value = std::nullopt;

  Bytes Serialize() const;
  static ResponsePayload Deserialize(const Bytes& bytes);
};

}  // namespace command

namespace client_redirect {

struct RequestPayload {
  NodeID reply_to;
  TCommand command;

  Bytes Serialize() const;
  static RequestPayload Deserialize(const Bytes& bytes);
};

}  // namespace client_redirect

}  // namespace distsysenv
