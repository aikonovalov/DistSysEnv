#include "event_manager.h"
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include "event.h"
#include "../node/context.h"
#include "../logger/logger.h"

namespace distsysenv {

SimulationClock EventManager::Now() const {
  return now_;
}

void EventManager::Schedule(const Event& event) {
  queue_.push(event);
}

std::optional<Event> EventManager::PopCurrentEvent() {
  if (queue_.empty()) {
    return std::nullopt;
  }

  Event event = queue_.top();
  queue_.pop();
  now_ = event.GetTimestamp();

  return event;
}

void EventManager::HandleEvent(const Event& curr_event) {
  NodeID target_id = id_manager_.GetNetworkID();
  
  if (curr_event.GetType() == EEventType::kMESSAGE_RECEIVE) {
    const auto& payload = std::get<MessageReceivePayload>(curr_event.GetPayload());
    target_id = payload.to_id;
  }
  else if (curr_event.GetType() == EEventType::kTIMER) {
    const auto& payload = std::get<TimerPayload>(curr_event.GetPayload());
    target_id = payload.node_id;
  }
  else if (curr_event.GetType() == EEventType::kMESSAGE_DROPPED) {
    return;
  }
  
  auto it = node_pool_.find(target_id);
  if (it != node_pool_.end() && it->second) {
    Context ctx(*this, target_id);
    it->second(curr_event, ctx);
  }
}

void EventManager::SetLogger(Logger&& logger) {
  logger_ = std::make_unique<Logger>(std::move(logger));
}

bool EventManager::Step(const std::function<bool(const Event&)>& exit_functor) {
  std::optional<Event> event = PopCurrentEvent();
  if (!event.has_value()) {
    return false;
  }

  if (exit_functor(*event)) {
    return false;
  }

  if (logger_ != nullptr) {
    (*logger_)(*event);
  }

  HandleEvent(*event);

  return true;
}

void EventManager::Process() {
  while (Step()) {}
}

void EventManager::ProcessUntil(SimulationClock until) {
  while (Step(
      [until](const Event& event) { return event.GetTimestamp() > until; })) {}
}

}  // namespace distsysenv
