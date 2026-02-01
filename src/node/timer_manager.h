#pragma once

#include <functional>
#include <string>
#include "../utils/node_id.h"
#include "../utils/utils.h"

namespace distsysenv {

class TimerManager {
 public:
  using TimerFunction = std::function<void(const std::string& timer_name,
                                           SimulationClock time_amount)>;

  explicit TimerManager(TimerFunction timer_function);

  void SetTimer(const std::string& timer_name, SimulationClock time_amount);

 private:
  TimerFunction timer_function_;
};

}  // namespace distsysenv
