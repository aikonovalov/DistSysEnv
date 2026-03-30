#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace distsysenv {

class SimulationTimerBook {
 public:
  using Token = uint64_t;

  Token Issue(const std::string& name);

  void Invalidate(const std::string& name);

  bool Validate(const std::string& name, Token token) const;

 private:
  std::unordered_map<std::string, Token> current_;
};

}  // namespace distsysenv
