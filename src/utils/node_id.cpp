#include "node_id.h"

namespace distsysenv {

NodeID::NodeID(Index index, Generation generation) : index_(index), generation_(generation) {}

NodeID::Index NodeID::IndexValue() const {
    return index_;
}

NodeID::Generation NodeID::GenerationValue() const {
    return generation_;
}

bool operator==(const NodeID& a, const NodeID& b) {
    return (a.IndexValue() == b.IndexValue()) && (a.GenerationValue() == b.GenerationValue());
}

bool operator!=(const NodeID& a, const NodeID& b) {
    return !(a == b);
}

bool operator<(const NodeID& a, const NodeID& b) {
    return (a.IndexValue() < b.IndexValue()) || (a.IndexValue() == b.IndexValue() && a.GenerationValue() < b.GenerationValue());
}

}