#include "event.h"

namespace distsysenv {

Event::Event(EEventType type, SimulationClock timestamp, EventPayload payload)
    : type_(type), timestamp_(timestamp), payload_(payload) {}

Event Event::MessageSend(SimulationClock at, NodeID from_id, NodeID to_id,
                         Message msg) {
  return Event(EEventType::kMESSAGE_SEND, at,
               MessageSendPayload{from_id, to_id, std::move(msg)});
}

Event Event::MessageReceive(SimulationClock at, NodeID from_id, NodeID to_id,
                            const Message& msg) {
  return Event(EEventType::kMESSAGE_RECEIVE, at,
               MessageReceivePayload{from_id, to_id, msg});
}

Event Event::NodeFail(SimulationClock at, NodeID node_id) {
  return Event(EEventType::kNODE_FAIL, at, NodeFailPayload{node_id});
}

Event Event::NodeRecover(SimulationClock at, NodeID node_id) {
  return Event(EEventType::kNODE_RECOVER, at, NodeRecoverPayload{node_id});
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
