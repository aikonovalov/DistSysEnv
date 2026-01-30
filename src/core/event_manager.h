#pragma once

#include <functional>
#include <optional>
#include <queue>
#include "../utils/utils.h"
#include "event.h"

namespace distsysenv {

using EventQueue = std::priority_queue<Event, std::vector<Event>, EventEarlier>;

class EventManager {
 public:
  SimulationClock Now() const;

  void Schedule(Event event);

  std::optional<Event> GetNext();

  void Process(const std::function<void(const Event&)>& handler);
  void ProcessUntil(SimulationClock until,
                    const std::function<void(const Event&)>& handler);

 private:
  SimulationClock now_ = 0;
  EventQueue queue_;
};

}  // namespace distsysenv
