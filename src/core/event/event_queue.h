#pragma once

#include <concepts>
#include <initializer_list>
#include <optional>
#include <queue>
#include <vector>
#include "event.h"

namespace distsysenv {

template <typename T, class TContainer = std::vector<T>,
          class TComparator = std::less<typename TContainer::value_type>>
class PriorityQueue {
 public:
  PriorityQueue() = default;

  bool empty() const { return container_.empty(); }
  size_t size() const { return container_.size(); }

  void push(const T& elem) {
    container_.push_back(elem);

    repair_up(size() - 1);
  }
  void push(T&& elem) {
    container_.push_back(std::move(elem));

    repair_up(size() - 1);
  }
  void push_range(std::initializer_list<T> list_of_elems) {
    container_.insert(container_.end(), list_of_elems);

    if (empty()) {
      return;
    }

    for (size_t i = container_.size() / 2 - 1; i-- > 0;) {
      repair_down(i);
    }
  }
  template <typename... Args>
  void emplace(Args&&... args) {
    container_.emplace_back(std::forward<Args>(args)...);

    repair_up(size() - 1);
  }

  void pop() {
    if (empty()) {
      return;
    }

    std::swap(container_.front(), container_.back());
    container_.pop_back();

    if (!container_.empty()) {
      repair_down(0);
    }
  }
  std::optional<T> extract() {
    if (empty()) {
      return std::nullopt;
    }

    T val = std::move(container_.front());

    pop();

    return val;
  }

 private:
  void repair_up(size_t curr_elem_idx) {
    while (curr_elem_idx > 0) {

      size_t parent = (curr_elem_idx - 1) / 2;

      if (!comp_(container_[parent], container_[curr_elem_idx])) {
        break;
      }

      std::swap(container_[parent], container_[curr_elem_idx]);
      curr_elem_idx = parent;
    }
  }

  void repair_down(size_t curr_elem_idx) {
    size_t n = container_.size();

    for (;;) {
      size_t left = curr_elem_idx * 2 + 1;
      size_t right = curr_elem_idx * 2 + 2;

      size_t winner = curr_elem_idx;

      if (left < n && comp_(container_[winner], container_[left])) {
        winner = left;
      }

      if (right < n && comp_(container_[winner], container_[right])) {
        winner = right;
      }

      if (winner == curr_elem_idx) {
        break;
      }

      std::swap(container_[curr_elem_idx], container_[winner]);
      curr_elem_idx = winner;
    }
  }

  TContainer container_;
  TComparator comp_;
};

using EventQueue = PriorityQueue<Event, std::vector<Event>, EventEarlier>;

}  // namespace distsysenv
