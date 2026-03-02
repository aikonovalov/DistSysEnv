#include "node_id_manager.h"
#include "node_id.h"

namespace distsysenv {

NodeID NodeIDManager::Generate() {
  Index index;

  if (!free_indices_.empty()) {
    index = free_indices_.front();
    free_indices_.pop();
  } else {
    index = next_index_;
    next_index_ = Index(static_cast<int32_t>(next_index_) + 1);
  }

  Generation generation{1};
  auto it = active_generations_.find(index);
  if (it != active_generations_.end()) {
    generation = Generation(static_cast<int32_t>(it->second) + 1);
  }

  active_generations_[index] = generation;

  return NodeID(index, generation, true);
}

NodeID NodeIDManager::GetNetworkID() const {
  return NodeID(Index{-1}, Generation{1}, true);
}

NodeID NodeIDManager::GetInvariantCheckerID() const {
  return NodeID(Index{-2}, Generation{1}, true);
}

void NodeIDManager::Release(const NodeID& id) {
  Index index = id.GetIndex();
  active_generations_.erase(index);
  free_indices_.push(index);
}

bool NodeIDManager::IsValid(const NodeID& id) const {
  Index index = id.GetIndex();
  auto it = active_generations_.find(index);
  return it != active_generations_.end() && it->second == id.GetGeneration();
}

}  // namespace distsysenv
