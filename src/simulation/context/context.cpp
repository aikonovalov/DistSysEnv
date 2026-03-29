#include "context.h"
#include <stdexcept>

#include "../event/event.h"

namespace distsysenv {

SimulationContext::SimulationContext(CoreContext& core,
                                     const SimulationContextOptions& options,
                                     SimulationTimerBook& timer_book)
    : core_(core), options_(options), timer_book_(timer_book) {}

TTime SimulationContext::Now() const {
  return core_.Now();
}

NodeID SimulationContext::GetOwnID() const {
  return core_.GetOwnID();
}

void SimulationContext::PushApplicationMessage(NodeID to, const Message& msg) {
  const TTime ts = core_.Now();
  MessageEventPayload payload{MessageDeliveryStatus::Sended, core_.GetOwnID(),
                              to, msg};
  if (options_.network_gateway.has_value()) {
    core_.PushEvent(MakeRoutedApplicationMessage(ts, *options_.network_gateway,
                                                 std::move(payload)));
  } else {
    core_.PushEvent(
        SimulationEvent::make<MessageEventPayload>::Of(ts, std::move(payload)));
  }
}

void SimulationContext::SendMessage(NodeID to, const Message& msg) {
  PushApplicationMessage(to, msg);
}

void SimulationContext::SendLocal(const Message& msg) {
  const NodeID own_id = core_.GetOwnID();

  if (!options_.checker_local_sink.has_value()) {
    return;
  }

  const NodeID terminal = *options_.checker_local_sink;
  if (terminal == own_id) {
    return;
  }

  core_.PushEvent(SimulationEvent::make<LocalMessageEventPayload>::Of(
      core_.Now(), terminal, LocalMessageEventPayload{own_id, msg}));
}

void SimulationContext::SendLocal(NodeID to, const Message& msg) {
  PushApplicationMessage(to, msg);
}

void SimulationContext::SetTimer(std::string name, TTime duration) {
  if (duration <= 0) {
    throw std::runtime_error("Timer duration must be positive");
  }

  const SimulationTimerBook::Token token = timer_book_.Issue(name);
  TimerEventPayload payload{core_.GetOwnID(), std::move(name), token};

  core_.PushEvent(SimulationEvent::make<TimerEventPayload>::Of(
      core_.Now() + duration, std::move(payload)));
}

void SimulationContext::CancelTimer(const std::string& name) {
  timer_book_.Invalidate(name);
}

bool SimulationContext::IsTimerValid(const std::string& name,
                                     SimulationTimerBook::Token token) const {
  return timer_book_.Validate(name, token);
}

CoreContext& SimulationContext::Core() {
  return core_;
}

}  // namespace distsysenv
