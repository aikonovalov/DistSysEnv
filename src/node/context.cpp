#include "context.h"
#include <cassert>
#include "../core/event.h"
#include "../core/event_manager.h"

namespace distsysenv {

Context::Context(EventManager& event_manager, NodeID own_id)
    : event_manager_(event_manager), id_(own_id) {}

void Context::Send(NodeID to_id, Message msg) {
  Event send_event =
      Event::MessageSend(event_manager_.Now(), id_, to_id, std::move(msg));

  ScheduleEvent(send_event);
}

void Context::SendLocal(Message msg) {
  event_manager_.SendToChecker(std::move(msg));
}

void Context::SetTimer(const TTimerName& timer_name, SimulationClock duration) {
  assert(duration > 0 && "Duration must be greater than zero");

  Event timer_event =
      Event::Timer(event_manager_.Now() + duration, id_, timer_name);

  ScheduleEvent(timer_event);
}

void Context::ScheduleEvent(const Event& event) {
  event_manager_.Schedule(event);
}

SimulationClock Context::Now() const {
  return event_manager_.Now();
}

NodeID Context::GetOwnID() const {
  return id_;
}

}  // namespace distsysenv
