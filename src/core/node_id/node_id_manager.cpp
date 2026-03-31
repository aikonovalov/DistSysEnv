#include "node_id_manager.h"
#include "node_id.h"

namespace distsysenv {

NodeID NodeIDManager::Generate() {
  Index_t index;

  if (!free_indices_.empty()) {
    index = *free_indices_.begin();

    free_indices_.erase(free_indices_.begin());
  } else {
    index = next_index_;

    next_index_ += 1;
  }

  Generation_t generation = 1;

  auto it = active_generations_.find(index);
  if (it != active_generations_.end()) {
    generation = it->second + 1;
  }

  active_generations_[index] = generation;

  return NodeID(Index{index}, Generation{generation});
}

void NodeIDManager::Release(NodeID id) {
  Index_t index = GetIndexVal(id.index());
  free_indices_.insert(index);
}

bool NodeIDManager::IsValid(NodeID id) const {
  Index_t index = GetIndexVal(id.index());

  if (free_indices_.contains(index)) {
    return false;
  }

  auto it = active_generations_.find(index);
  if (it == active_generations_.end()) {
    return false;
  }

  return it->second == static_cast<Generation_t>(id.generation());
}

}  // namespace distsysenv
