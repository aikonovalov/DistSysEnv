#pragma once

#include <queue>
#include <unordered_map>
#include "node_id.h"

namespace distsysenv {

class NodeIDManager {
 public:
  using Index = NodeID::Index;
  using Generation = NodeID::Generation;

  NodeIDManager() = default;

  NodeID Generate();

  NodeID GetNetworkID();

  void Release(const NodeID& id);

  bool IsValid(const NodeID& id) const;

 private:
  Index next_index_{1};
  std::unordered_map<Index, Generation> active_generations_;
  std::queue<Index> free_indices_;
};

}  // namespace distsysenv
