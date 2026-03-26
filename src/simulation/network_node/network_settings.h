#pragma once

#include <unordered_set>
#include "../core/node_id/node_id.h"
#include "../utils/time.h"

namespace distsysenv {

struct NetworkSettings {
  float drop_prob = 0.0f;
  TTime min_delay = 1;
  TTime max_delay = 10;
};

struct NodeNetworkSettings {
  bool is_failed = false;
  std::unordered_set<NodeID> partitioned_from;
};

}  // namespace distsysenv
