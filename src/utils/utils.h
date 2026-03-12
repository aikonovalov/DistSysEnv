#pragma once

#include <memory>
#include <vector>

namespace distsysenv {

using Byte = std::byte;
using Bytes = std::vector<Byte>;

using TOffset = int64_t;
using TTimerName = std::string;
using TIndex = int64_t;
using TCommand = std::string;

using SimulationClock = float;

template <typename T>
concept Serializable =
    (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>) ||
    requires(const T& v) {
      { v.Serialize() } -> std::same_as<Bytes>;
    } || requires(const T& v, Bytes& buf, TOffset offset) {
      { v.Serialize(buf, offset) } -> std::same_as<void>;
    };

template <typename T>
void append_item(Bytes& buffer, const T& value) {
  using TClear = std::remove_cvref_t<T>;

  if constexpr (std::is_trivially_copyable_v<TClear>) {
    size_t old = buffer.size();
    buffer.resize(old + sizeof(TClear));

    std::memcpy(buffer.data() + old, &value, sizeof(TClear));

  } else if constexpr (requires(const TClear& v) {
                         { v.Serialize() } -> std::same_as<Bytes>;
                       }) {
    Bytes tmp = value.Serialize();
    buffer.insert(buffer.end(), tmp.begin(), tmp.end());

  } else if constexpr (requires(const TClear& v, Bytes& buf, TOffset off) {
                         { v.Serialize(buf, off) } -> std::same_as<void>;
                       }) {
    value.Serialize(buffer, buffer.size());
  }
}

template <typename... Args>
void append(Bytes& buffer, const Args&... args) {
  (append_item(buffer, args), ...);
}

template <Serializable... Args>
Bytes BuildPayload(Args... args) {
  Bytes res_buffer;

  append(res_buffer, args...);

  return res_buffer;
}

}  // namespace distsysenv
