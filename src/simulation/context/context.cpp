#include "context.h"

#include "../event/event.h"

namespace distsysenv {

SimulationContext::SimulationContext(CoreContext& core,
                                     const SimulationContextOptions& options)
    : core_(core), options_(options) {}

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

void SimulationContext::SetTimer(std::string name, TTime fire_at) {
  TimerEventPayload payload{core_.GetOwnID(), std::move(name)};

  core_.PushEvent(SimulationEvent::make<TimerEventPayload>::Of(
      fire_at, std::move(payload)));
}

CoreContext& SimulationContext::Core() {
  return core_;
}

}  // namespace distsysenv
