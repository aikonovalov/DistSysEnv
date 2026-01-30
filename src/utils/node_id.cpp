#include "node_id.h"

namespace distsysenv {

NodeID::NodeID(Index index, Generation generation)
    : index_(index), generation_(generation) {}

NodeID::Index NodeID::GetIndex() const {
  return index_;
}

NodeID::Generation NodeID::GetGeneration() const {
  return generation_;
}

NodeID::Hash NodeID::GetHash() const {
  uint32_t index = static_cast<uint32_t>(static_cast<int32_t>(index_));
  uint32_t generation =
      static_cast<uint32_t>(static_cast<int32_t>(generation_));
  return Hash{static_cast<uint64_t>(index) << 32u | generation};
}

bool operator==(const NodeID& a, const NodeID& b) {
  return (a.GetIndex() == b.GetIndex()) &&
         (a.GetGeneration() == b.GetGeneration());
}

bool operator!=(const NodeID& a, const NodeID& b) {
  return !(a == b);
}

bool operator<(const NodeID& a, const NodeID& b) {
  return (a.GetIndex() < b.GetIndex()) ||
         (a.GetIndex() == b.GetIndex() &&
          a.GetGeneration() < b.GetGeneration());
}

}  // namespace distsysenv
