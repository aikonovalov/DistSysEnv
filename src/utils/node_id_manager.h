#pragma once

#include "node_id.h"
#include <queue>
#include <unordered_map>

namespace distsysenv {

class NodeIDManager {
public:
    using Index = NodeID::Index;
    using Generation = NodeID::Generation;

    NodeIDManager() = default;

    NodeID Generate();

    void Release(const NodeID& id);

    bool IsValid(const NodeID& id) const;

private:
    Index next_index_{0};
    std::unordered_map<Index, Generation> active_generations_;
    std::queue<Index> free_indices_;
};

} // namespace distsysenv
