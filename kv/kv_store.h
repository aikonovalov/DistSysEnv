#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include "../src/utils/utils.h"

#include "sizer.h"

namespace distsysenv {

template <Serializable TKey, Serializable TVal>
class Command {
 public:
  enum class Type : uint8_t {
    eGET,
    eSET,
    eDEL,
  };

  Command() = default;

  Command(Type type, TKey key, std::optional<TVal> value)
      : type_(type), key_(key), value_(value) {}

  Type type() const { return type_; }

  TKey key() const { return key_; }

  std::optional<TVal> value() const { return value_; }

  Bytes Serialize() const {
    Bytes buffer;
    append_item(buffer, type_);

    append_item(buffer, key_);

    if (type_ == Type::eSET) {
      append_item(buffer, value_);
    }

    return buffer;
  }

  static Command Deserialize(const Bytes& bytes) {
    Command cmd;

    TOffset offset = 0;

    read_field(bytes, offset, cmd.type_);
    read_field(bytes, offset, cmd.key_);
    if (cmd.type_ == Type::eSET) {
      read_field(bytes, offset, cmd.value_);
    }

    return cmd;
  }

 private:
  Type type_;
  TKey key_;
  std::optional<TVal> value_;
};

template <typename TKey, typename TVal>
class KVStore {
 public:
  KVStore() = default;

  std::optional<TVal> Get(const TKey& key) const {
    auto it = storage_.find(key);

    if (it == storage_.end()) {
      return std::nullopt;
    }

    return it->second;
  }

  Status Set(const TKey& key, const TVal& val) {
    storage_[key] = val;
    return Status::OK;
  }

  Status Delete(const TKey& key) {
    auto it = storage_.find(key);
    if (it == storage_.end()) {
      return Status::ERROR;
    }

    storage_.erase(it);
    return Status::OK;
  }

  size_t SpaceElapsed() const {
    size_t total = 0;
    for (const auto& [key, val] : storage_) {
      total += GetSize(key);
      total += GetSize(val);
    }

    return total;
  }

 private:
  std::map<TKey, TVal> storage_;
};

}  // namespace distsysenv
