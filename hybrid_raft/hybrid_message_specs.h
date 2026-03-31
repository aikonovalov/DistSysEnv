#pragma once

#include "../src/utils/utils.h"

namespace distsysenv::hybrid_msg {

namespace append_entries {

struct ResponsePayload {
  Status status;
  TIndex term;
  TIndex match_index;
  TIndex conflict_term = -1;
  TIndex conflict_index = -1;

  Bytes Serialize() const;
  static ResponsePayload Deserialize(const Bytes& bytes);
};

}  // namespace append_entries

}  // namespace distsysenv::hybrid_msg
