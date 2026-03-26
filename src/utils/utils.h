#pragma once

#include <cstring>
#include <type_traits>

#include "bytes.h"

namespace distsysenv {

using TOffset = int64_t;
using TIndex = int64_t;

enum class Status {
  OK,
  ERROR,
};

namespace utils {

template <typename T>
using TClearCVRef = std::remove_cvref_t<T>;

}

template <typename T>
concept SerializableContainer = std::ranges::range<utils::TClearCVRef<T>> &&
                                requires(const utils::TClearCVRef<T>& c) {
                                  typename utils::TClearCVRef<T>::value_type;
                                  { c.size() } -> std::convertible_to<size_t>;
                                };

template <typename T>
concept Serializable =
    (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>) ||
    requires(const T& v) {
      { v.Serialize() } -> std::same_as<Bytes>;
    } || requires(const T& v, Bytes& buf, TOffset offset) {
      { v.Serialize(buf, offset) } -> std::same_as<void>;
    } || SerializableContainer<T>;

template <typename T>
void append_item(Bytes& buffer, const T& value) {
  using TClearCVRef = utils::TClearCVRef<T>;

  if constexpr (std::is_trivially_copyable_v<TClearCVRef>) {
    size_t old = buffer.size();
    buffer.resize(old + sizeof(TClearCVRef));

    std::memcpy(buffer.data() + old, &value, sizeof(TClearCVRef));

  } else if constexpr (requires(const TClearCVRef& v) {
                         { v.Serialize() } -> std::same_as<Bytes>;
                       }) {
    Bytes tmp = value.Serialize();
    buffer.insert(buffer.end(), tmp.begin(), tmp.end());

  } else if constexpr (requires(const TClearCVRef& v, Bytes& buf, TOffset off) {
                         { v.Serialize(buf, off) } -> std::same_as<void>;
                       }) {
    value.Serialize(buffer, buffer.size());
  } else if constexpr (SerializableContainer<TClearCVRef>) {
    size_t size = value.size();
    append_item(buffer, size);

    for (const auto& elem : value) {
      append_item(buffer, elem);
    }
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

template <typename T>
void read_field(const Bytes& buffer, TOffset& offset, T& out) {
  if constexpr (std::is_trivially_copyable_v<T>) {
    std::memcpy(&out, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);

  } else if constexpr (requires(const Bytes& b) {
                         utils::TClearCVRef<T>::Deserialize(b);
                       }) {
    Bytes tail(buffer.begin() + offset, buffer.end());
    out = utils::TClearCVRef<T>::Deserialize(tail);
    offset = buffer.size();

  } else if constexpr (SerializableContainer<T>) {
    size_t size;
    read_field(buffer, offset, size);
    out.resize(size);

    for (size_t i = 0; i < size; ++i) {
      read_field(buffer, offset, out[i]);
    }

  } else {
    throw std::runtime_error("Unsupported type for read_field");
  }
}

}  // namespace distsysenv
