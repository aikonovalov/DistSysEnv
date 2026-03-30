#pragma once

#include "../../utils/utils.h"

#include <concepts>
#include <type_traits>

namespace distsysenv {

using MessageType = std::string;

class Message {
 private:
  Message(MessageType&& type, Bytes&& payload);

 public:
  static Message FromDescription(MessageType type, Bytes payload);
  static Message Deserialize(const Bytes& serialized_message);

  Bytes Serialize() const;

  const MessageType& GetType() const;
  const Bytes& GetPayload() const;

 private:
  MessageType type_;
  Bytes payload_;
};

}  // namespace distsysenv
