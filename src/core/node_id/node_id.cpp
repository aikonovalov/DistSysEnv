#include "node_id.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace distsysenv {

NodeID::NodeID(Index index, Generation generation)
    : index_(GetIndexVal(index)), generation_(GetGenVal(generation)) {
  assert(index_ >= 0 && "Index must be non negative");
}

Index NodeID::index() const {
  return Index{index_};
}

Generation NodeID::generation() const {
  return Generation{generation_};
}

NodeID NodeID::Deserialize(const Bytes& buffer, TOffset offset) {
  Index_t index;
  read_field(buffer, offset, index);

  Generation_t gen;
  read_field(buffer, offset, gen);

  return NodeID{Index{index}, Generation{gen}};
}

void NodeID::Serialize(Bytes& buffer, TOffset offset) const {
  const TOffset need = offset + EncodedSize();

  if (buffer.size() < need) {
    buffer.resize(need);
  }

  std::memcpy(buffer.data() + offset, &index_, sizeof(index_));
  std::memcpy(buffer.data() + offset + sizeof(index_), &generation_,
              sizeof(generation_));
}

uint64_t NodeID::GetHash() const {
  uint64_t result = index_;
  result = (result << 32u) + generation_;

  return result;
}

bool operator==(NodeID a, NodeID b) {
  return (a.index() == b.index()) && (a.generation() == b.generation());
}

bool operator!=(NodeID a, NodeID b) {
  return !(a == b);
}

bool operator<(NodeID a, NodeID b) {
  return (a.index() < b.index()) ||
         (a.index() == b.index() && a.generation() < b.generation());
}

}  // namespace distsysenv
