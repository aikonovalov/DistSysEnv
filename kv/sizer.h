#pragma once

#include <concepts>
namespace distsysenv {

template <typename T>
concept HasSize = requires(T item) {
  { item.size() } -> std::convertible_to<size_t>;
};

template <typename T>
concept HasCapacity = requires(T item) {
  { item.capacity() } -> std::convertible_to<size_t>;
};

template <typename T>
size_t GetSize(const T& obj) {
  if constexpr (HasSize<T>) {
    return obj.size() + sizeof(T);
  } else if constexpr (HasCapacity<T>) {
    return obj.capacity() + sizeof(T);
  }

  return sizeof(T);
}

}  // namespace distsysenv
