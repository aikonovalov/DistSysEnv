#include "event_manager.h"
#include <functional>
#include <optional>
#include "../context/context.h"
#include "event.h"

namespace distsysenv {

TTime EventManager::Now() const {
  return now_;
}

void EventManager::PushEvent(const Event& event) {
  queue_.push(event);
}

void EventManager::HandleEvent(const Event& event) {
  auto it = node_pool_.find(event.to);
  if (it != node_pool_.end() && it->second) {
    CoreContext ctx(*this, event.to);

    it->second(event, ctx);
  }
}

Status EventManager::Step(
    const std::function<bool(const Event&)>& exit_functor) {
  std::optional<Event> event = queue_.extract();
  if (!event.has_value()) {
    return Status::ERROR;
  }

  if (exit_functor(*event)) {
    queue_.push(*event);
    return Status::ERROR;
  }

  now_ = event.value().timestamp;
  HandleEvent(event.value());

  return Status::OK;
}

void EventManager::Process() {
  for (Status curr_status = Step(); curr_status == Status::OK;
       curr_status = Step()) {}
}

void EventManager::ProcessUntil(TTime until) {
  auto exit_func = [until](const Event& event) {
    return event.timestamp > until;
  };

  for (Status curr_status = Step(exit_func); curr_status == Status::OK;
       curr_status = Step(exit_func)) {}
}

NodeID EventManager::RegisterNode(EventHandler handler) {
  NodeID id = id_manager_.Generate();
  node_pool_[id] = std::move(handler);

  return id;
}

}  // namespace distsysenv
