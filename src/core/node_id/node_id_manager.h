#pragma once

#include <queue>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include "node_id.h"

namespace distsysenv {

class NodeIDManager {
 public:
  NodeID Generate();
  void Release(NodeID id);
  bool IsValid(NodeID id) const;

 private:
  Index_t next_index_ = 0;
  std::unordered_map<Index_t, Generation_t> active_generations_;
  std::unordered_set<Index_t> free_indices_;
};

}  // namespace distsysenv
