#pragma once

#include "../utils/utils.h"
#include "../utils/node_id.h"
#include <unordered_set>

namespace distsysenv {

struct NetworkSettings {
  float drop_prob = 0.0f;
  SimulationClock min_delay = 1;
  SimulationClock max_delay = 10;
};

struct NodeNetworkSettings {
  bool is_failed = false;
  std::unordered_set<NodeID> partitioned_from;
};

}  // namespace distsysenv
