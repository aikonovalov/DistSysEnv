#pragma once

#include <cstdint>
#include <functional>
#include "../../utils/bytes.h"
#include "../../utils/utils.h"

namespace distsysenv {

enum class Index : int32_t;
using Index_t = std::underlying_type_t<Index>;
static constexpr Index_t GetIndexVal(Index index) {
  return static_cast<Index_t>(index);
}

enum class Generation : int32_t;
using Generation_t = std::underlying_type_t<Generation>;
static constexpr Generation_t GetGenVal(Generation gen) {
  return static_cast<Index_t>(gen);
}

class NodeID {
 public:
  NodeID() = delete;
  NodeID(Index index, Generation generation);

  Index index() const;
  Generation generation() const;

  static NodeID Deserialize(const Bytes& buffer, TOffset offset);
  void Serialize(Bytes& buffer, TOffset offset) const;

  uint64_t GetHash() const;

  friend bool operator==(NodeID a, NodeID b);
  friend bool operator!=(NodeID a, NodeID b);
  friend bool operator<(NodeID a, NodeID b);

 private:
  static constexpr TOffset EncodedSize() {
    return sizeof(Index) + sizeof(Generation);
  }

  Index_t index_;
  Generation_t generation_;
};

}  // namespace distsysenv

namespace std {

template <>
struct hash<distsysenv::NodeID> {
  size_t operator()(distsysenv::NodeID id) const noexcept {
    return id.GetHash();
  }
};

}  // namespace std
