#include "logger.h"

namespace distsysenv {

void Logger::RegisterHandler(EEventType event_type,
                             std::function<void(const Event&)>&& handler) {
  event_handlers_[event_type] = std::move(handler);
}

void Logger::operator()(const Event& event) {
  EEventType curr_event_type = event.GetType();

  auto it = event_handlers_.find(curr_event_type);

  if (it == event_handlers_.end()) {
    return;
  }

  it->second(event);
}

}  // namespace distsysenv