#pragma once

#include "utils.h"

namespace distsysenv::core {

using MessageType = std::string;

class Message {
private:
    using LengthEncoding = uint64_t;

    Message(std::string&& type, Bytes&& payload);

public:
    static Message FromDescription(std::string type, Bytes payload);
    static Message FromBytes(const Bytes& serialized_message);

    const std::string& GetType() const;
    const Bytes& GetPayload() const;

    Bytes Serialize() const;

private:
    void StoreToBuffer(LengthEncoding src_size, const void* src, Bytes* dst, size_t& current_offset) const;

    std::string type_;
    Bytes payload_;
};

} // namespace distsysenv::core
