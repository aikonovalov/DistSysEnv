#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

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

template <typename>
inline constexpr bool kAlwaysFalse = false;

template <typename T>
struct IsStdOptional : std::false_type {};

template <typename U>
struct IsStdOptional<std::optional<U>> : std::true_type {};

template <typename T>
inline constexpr bool kIsStdOptionalV = IsStdOptional<TClearCVRef<T>>::value;

}  // namespace utils

template <typename T>
concept SerializableContainer = std::ranges::range<utils::TClearCVRef<T>> &&
                                requires(const utils::TClearCVRef<T>& c) {
                                  typename utils::TClearCVRef<T>::value_type;
                                  { c.size() } -> std::convertible_to<size_t>;
                                };

template <typename T>
concept Serializable = []() {
  using U = utils::TClearCVRef<T>;

  if constexpr (std::is_trivially_copyable_v<U> && !std::is_pointer_v<U>) {
    return true;

  } else if constexpr (requires(const U& v) {
                         { v.Serialize() } -> std::same_as<Bytes>;
                       }) {
    return true;

  } else if constexpr (requires(const U& v, Bytes& buf, TOffset offset) {
                         { v.Serialize(buf, offset) } -> std::same_as<void>;
                       }) {
    return true;

  } else if constexpr (SerializableContainer<T>) {
    return true;

  } else if constexpr (utils::kIsStdOptionalV<T>) {
    return SerializableImpl<typename U::value_type>();
  }

  return false;
}();

template <typename T>
void append_item(Bytes& buffer, const T& value) {
  using TClearCVRef = utils::TClearCVRef<T>;

  if constexpr (requires(const TClearCVRef& v) {
                  { v.Serialize() } -> std::same_as<Bytes>;
                }) {
    Bytes tmp = value.Serialize();
    buffer.insert(buffer.end(), tmp.begin(), tmp.end());

  } else if constexpr (requires(const TClearCVRef& v, Bytes& buf, TOffset off) {
                         { v.Serialize(buf, off) } -> std::same_as<void>;
                       }) {
    value.Serialize(buffer, buffer.size());

  } else if constexpr (utils::kIsStdOptionalV<T>) {
    const uint8_t has_value = value.has_value() ? 1 : 0;
    append_item(buffer, has_value);

    if (value.has_value()) {
      append_item(buffer, *value);
    }

  } else if constexpr (std::is_trivially_copyable_v<TClearCVRef> &&
                       !std::is_pointer_v<TClearCVRef>) {
    size_t old = buffer.size();
    buffer.resize(old + sizeof(TClearCVRef));

    std::memcpy(buffer.data() + old, &value, sizeof(TClearCVRef));

  } else if constexpr (SerializableContainer<TClearCVRef>) {
    size_t size = value.size();
    append_item(buffer, size);

    for (const auto& elem : value) {
      append_item(buffer, elem);
    }

  } else {
    static_assert(utils::kAlwaysFalse<TClearCVRef>, "type not supported");
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
  if constexpr (requires(const Bytes& buf, TOffset& off) {
                  {
                    utils::TClearCVRef<T>::Deserialize(buf, off)
                  } -> std::same_as<utils::TClearCVRef<T>>;
                }) {
    out = utils::TClearCVRef<T>::Deserialize(buffer, offset);

  } else if constexpr (utils::kIsStdOptionalV<T>) {
    using U = typename utils::TClearCVRef<T>::value_type;

    uint8_t has_value{};
    read_field(buffer, offset, has_value);

    if (has_value == 1) {
      if constexpr (requires(const Bytes& buf, TOffset& off) {
                      { U::Deserialize(buf, off) } -> std::same_as<U>;
                    }) {
        out.emplace(U::Deserialize(buffer, offset));

      } else {
        U inner{};
        read_field(buffer, offset, inner);

        out = std::move(inner);
      }

      return;
    }

    out = std::nullopt;

  } else if constexpr (std::is_trivially_copyable_v<T> &&
                       !std::is_pointer_v<T>) {
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

template <typename... Args>
std::tuple<Args...> ReadFields(const Bytes& buffer, TOffset& offset) {
  std::tuple<Args...> out{};
  std::apply(
      [&](auto&... fields) { (read_field(buffer, offset, fields), ...); }, out);

  return out;
}

}  // namespace distsysenv
