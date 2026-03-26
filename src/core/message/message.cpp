#include "message.h"

namespace distsysenv {

Message::Message(MessageType&& type, Bytes&& payload)
    : type_(std::move(type)), payload_(std::move(payload)) {}

Message Message::FromDescription(MessageType type, Bytes payload) {
  return Message(std::move(type), std::move(payload));
}

Bytes Message::Serialize() const {
  return BuildPayload(type_, payload_);
}

Message Message::Deserialize(const Bytes& serialized_message) {
  TOffset offset = 0;

  MessageType type;
  read_field(serialized_message, offset, type);

  Bytes payload;
  read_field(serialized_message, offset, payload);

  return FromDescription(std::move(type), std::move(payload));
}

const MessageType& Message::GetType() const {
  return type_;
}

const Bytes& Message::GetPayload() const {
  return payload_;
}

}  // namespace distsysenv
