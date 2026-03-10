#pragma once

#include "utils.h"

namespace distsysenv {

using MessageType = std::string;

class Message {
 private:
  using LengthEncoding = int64_t;

  Message(MessageType&& type, Bytes&& payload);

 public:
  static Message FromDescription(MessageType type, Bytes payload);
  static Message FromBytes(const Bytes& serialized_message);

  const MessageType& GetType() const;
  const Bytes& GetPayload() const;

  Bytes Serialize() const;

 private:
  MessageType type_;
  Bytes payload_;
};

}  // namespace distsysenv
