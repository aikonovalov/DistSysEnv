#include "event_manager.h"
#include <optional>
#include "event.h"

namespace distsysenv {

SimulationClock EventManager::Now() const {
  return now_;
}

void EventManager::Schedule(Event event) {
  queue_.push(std::move(event));
}

std::optional<Event> EventManager::GetNext() {
  if (queue_.empty()) {
    return std::nullopt;
  }

  Event event = queue_.top();
  queue_.pop();
  now_ = event.GetTimestamp();

  return event;
}

void EventManager::Process(const std::function<void(const Event&)>& handler) {
  std::optional<Event> event = GetNext();

  while (event.has_value()) {
    handler(*event);
    event = GetNext();
  }
}

void EventManager::ProcessUntil(
    SimulationClock until, const std::function<void(const Event&)>& handler) {
  while (!queue_.empty()) {
    std::optional<Event> event = GetNext();
    if (!event.has_value()) {
      return;
    }

    if (event->GetTimestamp() > until) {
      return;
    }

    handler(*event);
  }
}

}  // namespace distsysenv
