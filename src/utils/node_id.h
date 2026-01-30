#pragma once

#include <cstdint>

namespace distsysenv {

class NodeID {
public:
    enum class Index : int32_t;
    enum class Generation : int32_t;

    NodeID() = delete;

    Index GetIndex() const;

    Generation GetGeneration() const;

    friend bool operator==(const NodeID& a, const NodeID& b);
    
    friend bool operator!=(const NodeID& a, const NodeID& b);

    friend bool operator<(const NodeID& a, const NodeID& b);

private:
    friend class NodeIDManager;

    NodeID(Index index, Generation generation);

    Index index_;
    Generation generation_;
};

} // namespace distsysenv

