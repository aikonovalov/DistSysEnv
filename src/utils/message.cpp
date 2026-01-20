#include "message.h"
#include <cstring>

namespace distsysenv {

static constexpr char kDEFAULT_FILLER = '\0';

Message::Message(std::string&& type, Bytes&& payload) : type_(std::move(type)), payload_(std::move(payload)) {}

Message Message::FromDescription(std::string type, Bytes payload) {
    return Message(std::move(type), std::move(payload));
}

void Message::StoreToBuffer(LengthEncoding src_size, const void* src, Bytes* dst, size_t& current_offset) const {
    std::memcpy(dst->data() + current_offset, &src_size, sizeof(LengthEncoding));
    current_offset += sizeof(LengthEncoding);

    std::memcpy(dst->data() + current_offset, src, src_size);
    current_offset += src_size;
}

Bytes Message::Serialize() const {
    Bytes buffer(sizeof(LengthEncoding) + type_.size() + sizeof(LengthEncoding) + payload_.size());

    size_t current_offset = 0;

    StoreToBuffer(type_.size(), type_.data(), &buffer, current_offset);
    StoreToBuffer(payload_.size(), payload_.data(), &buffer, current_offset);

    return buffer;
}

Message Message::FromBytes(const Bytes& serialized_message) {
    if (serialized_message.size() < sizeof(LengthEncoding) + sizeof(LengthEncoding)) {
        throw std::runtime_error("Invalid serialized view of message: too short");
    }

    size_t current_offset = 0;

    LengthEncoding type_size;
    std::memcpy(&type_size, serialized_message.data() + current_offset, sizeof(LengthEncoding));
    current_offset += sizeof(LengthEncoding);

    if (serialized_message.size() < sizeof(LengthEncoding) + type_size + sizeof(LengthEncoding)) {
        throw std::runtime_error("Invalid serialized view of message: type size mismatching");
    }

    std::string type(type_size, kDEFAULT_FILLER);
    if (type_size > 0) {
        std::memcpy(type.data(), serialized_message.data() + current_offset, type_size);
    }
    current_offset += type_size;

    LengthEncoding payload_size;
    std::memcpy(&payload_size, serialized_message.data() + current_offset, sizeof(LengthEncoding));
    current_offset += sizeof(LengthEncoding);

    if (serialized_message.size() != sizeof(LengthEncoding) + type_size + sizeof(LengthEncoding) + payload_size) {
        throw std::runtime_error("Invalid serialized view of message");
    }

    Bytes payload(payload_size);
    if (payload_size > 0) {
        std::memcpy(payload.data(), serialized_message.data() + current_offset, payload_size);
    }
    current_offset += payload_size;

    return FromDescription(std::move(type), std::move(payload));
}

} // namespace distsysenv
