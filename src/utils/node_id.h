#pragma once

#include <cstdint>
#include <functional>
#include "utils.h"

namespace distsysenv {

class NodeID {
 public:
  enum class Index : int32_t;
  enum class Generation : int32_t;
  enum class Hash : uint64_t;

  NodeID() = delete;

  NodeID(Index index, Generation generation);

  Index GetIndex() const;

  Generation GetGeneration() const;

  Hash GetHash() const;

  static constexpr size_t EncodedSize() {
    return sizeof(Index) + sizeof(Generation);
  }

  static NodeID DecodeFromBytes(const Bytes& buffer, TOffset offset);

  void StoreToBuffer(Bytes& buffer, TOffset offset) const;

  friend bool operator==(const NodeID& a, const NodeID& b);

  friend bool operator!=(const NodeID& a, const NodeID& b);

  friend bool operator<(const NodeID& a, const NodeID& b);

 private:
  friend class NodeIDManager;

  NodeID(Index index, Generation generation, bool allow_negative_index);

  Index index_;
  Generation generation_;
};

}  // namespace distsysenv

namespace std {

template <>
struct hash<distsysenv::NodeID> {
  std::size_t operator()(const distsysenv::NodeID& id) const noexcept {
    return static_cast<std::size_t>(id.GetHash());
  }
};

template <>
struct hash<distsysenv::NodeID::Index> {
  std::size_t operator()(distsysenv::NodeID::Index i) const noexcept {
    return static_cast<std::size_t>(static_cast<int32_t>(i));
  }
};

}  // namespace std
