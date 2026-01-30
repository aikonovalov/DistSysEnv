#include "event.h"

namespace distsysenv {

Event::Event(EEventType type, SimulationClock timestamp, EventPayload payload)
    : type_(type), timestamp_(timestamp), payload_(payload) {}

Event Event::MessageSend(SimulationClock at, NodeID from, NodeID to,
                         Message msg) {
  return Event(EEventType::kMESSAGE_SEND, at,
               MessageSendPayload{from, to, std::move(msg)});
}

Event Event::MessageReceive(SimulationClock at, NodeID from, NodeID to,
                            const Message& msg) {
  return Event(EEventType::kMESSAGE_RECEIVE, at,
               MessageReceivePayload{from, to, msg});
}

Event Event::NodeFail(SimulationClock at, NodeID node) {
  return Event(EEventType::kNODE_FAIL, at, NodeFailPayload{node});
}

Event Event::NodeRecover(SimulationClock at, NodeID node) {
  return Event(EEventType::kNODE_RECOVER, at, NodeRecoverPayload{node});
}

const EEventType& Event::GetType() const {
  return type_;
}

const SimulationClock& Event::GetTimestamp() const {
  return timestamp_;
}

const EventPayload& Event::GetPayload() const {
  return payload_;
}

bool EventEarlier::operator()(const Event& a, const Event& b) const {
  return std::tie(a.GetTimestamp(), a.GetType()) >
         std::tie(b.GetTimestamp(), b.GetType());
}

}  // namespace distsysenv