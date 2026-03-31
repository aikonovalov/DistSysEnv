#include "hybrid_message_specs.h"

namespace distsysenv::hybrid_msg::append_entries {

Bytes ResponsePayload::Serialize() const {
  return BuildPayload(status, term, match_index, conflict_term, conflict_index);
}

ResponsePayload ResponsePayload::Deserialize(const Bytes& bytes) {
  TOffset offset = 0;

  Status status{};
  TIndex term{};
  TIndex match_index{};
  TIndex conflict_term = -1;
  TIndex conflict_index = -1;

  read_field(bytes, offset, status);
  read_field(bytes, offset, term);
  read_field(bytes, offset, match_index);
  read_field(bytes, offset, conflict_term);
  read_field(bytes, offset, conflict_index);

  return ResponsePayload{
      .status = status,
      .term = term,
      .match_index = match_index,
      .conflict_term = conflict_term,
      .conflict_index = conflict_index,
  };
}

}  // namespace distsysenv::hybrid_msg::append_entries
