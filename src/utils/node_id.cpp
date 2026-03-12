#include "node_id.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace distsysenv {

NodeID::NodeID(Index index, Generation generation)
    : index_(index), generation_(generation) {
  assert(static_cast<int32_t>(index) >= 0 && "Index must be non negative");
}

NodeID::NodeID(Index index, Generation generation, bool allow_negative)
    : index_(index), generation_(generation) {
  (void)allow_negative;
}

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

NodeID NodeID::Deserialize(const Bytes& buffer, TOffset offset) {
  assert(offset >= 0 &&
         static_cast<size_t>(offset) + EncodedSize() <= buffer.size() &&
         "NodeID::Deserialize buffer overflow");

  Index index;
  Generation gen;

  std::memcpy(&index, buffer.data() + offset, sizeof(index));
  std::memcpy(&gen, buffer.data() + offset + sizeof(index), sizeof(gen));

  return NodeID{index, gen};
}

void NodeID::Serialize(Bytes& buffer, TOffset offset) const {
  assert(offset >= 0 && "NodeID::Serialize negative offset");

  const size_t need = static_cast<size_t>(offset) + EncodedSize();

  if (buffer.size() < need) {
    buffer.resize(need);
  }

  const Index index = index_;
  const Generation gen = generation_;

  std::memcpy(buffer.data() + offset, &index, sizeof(index));
  std::memcpy(buffer.data() + offset + sizeof(index), &gen, sizeof(gen));
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
