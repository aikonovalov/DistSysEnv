#pragma once

#include "utils.h"

#include <concepts>
#include <type_traits>

namespace distsysenv {

using MessageType = std::string;

template <typename T>
concept Serializable =
    (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>) ||
    requires(const T& v, Bytes& buf) {
      { v.Serialize(buf) } -> std::same_as<void>;
    };

class Message {
 private:
  using LengthEncoding = int64_t;

  Message(MessageType&& type, Bytes&& payload);

 public:
  template <Serializable... Args>
  Message(MessageType type, const Args&... args) : type_(std::move(type)) {
    append(args...);
  }

  static Message FromDescription(MessageType type, Bytes payload);
  static Message Deserialize(const Bytes& serialized_message);

  const MessageType& GetType() const;
  const Bytes& GetPayload() const;

  Bytes Serialize() const;

 private:
  template <typename T>
  void append_item(const T& value) {
    using TClear = std::remove_cvref_t<T>;

    if constexpr (std::is_trivially_copyable_v<TClear>) {
      size_t old = payload_.size();
      payload_.resize(old + sizeof(TClear));

      std::memcpy(payload_.data() + old, &value, sizeof(TClear));

    } else {
      value.Serialize(payload_);
    }
  }

  template <typename... Args>
  void append(const Args&... args) {
    (append_item(args), ...);
  }

  MessageType type_;
  Bytes payload_;
};

}  // namespace distsysenv
