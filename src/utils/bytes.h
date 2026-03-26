#pragma once

#include <cstddef>
#include <vector>

namespace distsysenv {

using Byte = std::byte;

class Bytes {
 private:
  using Storage = std::vector<Byte>;

 public:
  using value_type = Storage::value_type;
  using iterator = Storage::iterator;
  using const_iterator = Storage::const_iterator;

  Bytes() = default;
  explicit Bytes(size_t size) : data_(size) {}
  Bytes(size_t size, Byte val) : data_(size, val) {}
  Bytes(const_iterator begin, const_iterator end) : data_(begin, end) {}

  auto begin() { return data_.begin(); }
  auto begin() const { return data_.begin(); }
  auto end() { return data_.end(); }
  auto end() const { return data_.end(); }

  size_t size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }

  void resize(size_t new_size) { data_.resize(new_size); }
  void reserve(size_t size_to_reserve) { data_.reserve(size_to_reserve); }

  Byte* data() { return data_.data(); }
  const Byte* data() const { return data_.data(); }
  Byte& operator[](size_t idx) { return data_[idx]; }
  const Byte& operator[](size_t idx) const { return data_[idx]; }

  void insert(const_iterator position, const_iterator other_begin,
              const_iterator other_end) {
    data_.insert(position, other_begin, other_end);
  }

 private:
  Storage data_;
};

}  // namespace distsysenv
