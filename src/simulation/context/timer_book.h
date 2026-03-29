#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace distsysenv {

class SimulationTimerBook {
 public:
  using Token = uint64_t;

  Token Issue(const std::string& name);

  /// Bumps generation for `name` so all already-queued timer events for that name
  /// become stale (no new event is scheduled). Same effect as SetTimer with a new
  /// token, but without scheduling a fire.
  void Invalidate(const std::string& name);

  bool Validate(const std::string& name, Token token) const;

 private:
  std::unordered_map<std::string, Token> current_;
};

}  // namespace distsysenv
