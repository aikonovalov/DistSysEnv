#pragma once

#include <map>
#include <optional>

#include "sizer.h"

namespace distsysenv {

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

  enum class Status { Ok, Error };
  Status Set(const TKey& key, const TVal& val) {
    storage_[key] = val;
    return Status::Ok;
  }

  Status Delete(const TKey& key) {
    auto it = storage_.find(key);
    if (it == storage_.end()) {
      return Status::Error;
    }
    
    storage_.erase(it);
    return Status::Ok;
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