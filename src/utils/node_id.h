#pragma once

#include <cstdint>

namespace distsysenv {

struct NodeID {
private:
    friend class Simulator;

    using Index = uint32_t;
    using Generation = uint32_t;

    NodeID(Index index, Generation generation);

public:
    NodeID() = delete;

    Index IndexValue() const;

    Generation GenerationValue() const;

    friend bool operator==(const NodeID& a, const NodeID& b);
    
    friend bool operator!=(const NodeID& a, const NodeID& b);

    friend bool operator<(const NodeID& a, const NodeID& b);

private:
    Index index_;
    Generation generation_;
};

} // namespace distsysenv

