#include "message.h"
#include <cstring>

namespace distsysenv {

static constexpr char kDEFAULT_FILLER = '\0';

Message::Message(MessageType&& type, Bytes&& payload)
    : type_(std::move(type)), payload_(std::move(payload)) {}

Message Message::FromDescription(MessageType type, Bytes payload) {
  return Message(std::move(type), std::move(payload));
}

Bytes Message::Serialize() const {
  Bytes buffer(sizeof(LengthEncoding) + type_.size() + sizeof(LengthEncoding) +
               payload_.size());
  TOffset offset = 0;

  const LengthEncoding type_size = static_cast<LengthEncoding>(type_.size());
  std::memcpy(buffer.data() + offset, &type_size, sizeof(LengthEncoding));
  offset += sizeof(LengthEncoding);
  std::memcpy(buffer.data() + offset, type_.data(), type_size);
  offset += type_size;

  const LengthEncoding payload_size =
      static_cast<LengthEncoding>(payload_.size());
  std::memcpy(buffer.data() + offset, &payload_size, sizeof(LengthEncoding));
  offset += sizeof(LengthEncoding);
  std::memcpy(buffer.data() + offset, payload_.data(), payload_size);

  return buffer;
}

Message Message::Deserialize(const Bytes& serialized_message) {
  if (serialized_message.size() <
      sizeof(LengthEncoding) + sizeof(LengthEncoding)) {
    throw std::runtime_error("Invalid serialized view of message: too short");
  }

  TOffset current_offset = 0;

  LengthEncoding type_size;
  std::memcpy(&type_size, serialized_message.data() + current_offset,
              sizeof(LengthEncoding));

  current_offset += sizeof(LengthEncoding);

  if (serialized_message.size() <
      sizeof(LengthEncoding) + type_size + sizeof(LengthEncoding)) {
    throw std::runtime_error(
        "Invalid serialized view of message: type size mismatching");
  }

  MessageType type(type_size, kDEFAULT_FILLER);
  if (type_size > 0) {
    std::memcpy(type.data(), serialized_message.data() + current_offset,
                type_size);
  }

  current_offset += type_size;

  LengthEncoding payload_size;
  std::memcpy(&payload_size, serialized_message.data() + current_offset,
              sizeof(LengthEncoding));

  current_offset += sizeof(LengthEncoding);

  if (serialized_message.size() != sizeof(LengthEncoding) + type_size +
                                       sizeof(LengthEncoding) + payload_size) {
    throw std::runtime_error("Invalid serialized view of message");
  }

  Bytes payload(payload_size);
  if (payload_size > 0) {
    std::memcpy(payload.data(), serialized_message.data() + current_offset,
                payload_size);
  }

  current_offset += payload_size;

  return FromDescription(std::move(type), std::move(payload));
}

const MessageType& Message::GetType() const {
  return type_;
}

const Bytes& Message::GetPayload() const {
  return payload_;
}

}  // namespace distsysenv
