#include "timer_manager.h"

namespace distsysenv {

TimerManager::TimerManager(TimerFunction timer_function)
    : timer_function_(std::move(timer_function)) {}

void TimerManager::SetTimer(const std::string& timer_name,
                            SimulationClock time_amount) {
  timer_function_(timer_name, time_amount);
}

}  // namespace distsysenv
