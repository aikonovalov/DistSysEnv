#pragma once

#include <cstdint>
#include <functional>

namespace distsysenv {

class NodeID {
 public:
  enum class Index : int32_t;
  enum class Generation : int32_t;
  enum class Hash : uint64_t;

  NodeID() = delete;

  Index GetIndex() const;

  Generation GetGeneration() const;

  Hash GetHash() const;

  friend bool operator==(const NodeID& a, const NodeID& b);

  friend bool operator!=(const NodeID& a, const NodeID& b);

  friend bool operator<(const NodeID& a, const NodeID& b);

 private:
  friend class NodeIDManager;

  NodeID(Index index, Generation generation);

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
