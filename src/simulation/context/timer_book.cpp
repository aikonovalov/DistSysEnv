#include "timer_book.h"

namespace distsysenv {

SimulationTimerBook::Token SimulationTimerBook::Issue(const std::string& name) {
  return ++current_[name];
}

void SimulationTimerBook::Invalidate(const std::string& name) {
  ++current_[name];
}

bool SimulationTimerBook::Validate(const std::string& name, Token token) const {
  const auto it = current_.find(name);
  return it != current_.end() && it->second == token;
}

}  // namespace distsysenv
