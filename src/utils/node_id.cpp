#include "node_id.h"

namespace distsysenv {

NodeID::NodeID(Index index, Generation generation) : index_(index), generation_(generation) {}

NodeID::Index NodeID::GetIndex() const {
    return index_;
}

NodeID::Generation NodeID::GetGeneration() const {
    return generation_;
}

bool operator==(const NodeID& a, const NodeID& b) {
    return (a.GetIndex() == b.GetIndex()) && (a.GetGeneration() == b.GetGeneration());
}

bool operator!=(const NodeID& a, const NodeID& b) {
    return !(a == b);
}

bool operator<(const NodeID& a, const NodeID& b) {
    return (a.GetIndex() < b.GetIndex()) || (a.GetIndex() == b.GetIndex() && a.GetGeneration() < b.GetGeneration());
}

}